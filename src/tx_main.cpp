#include <Arduino.h>
#include <EEPROM.h>
#include "crc8.h"
#include "CrsfSerial.h"
#include "crsf_protocol.h"
#include "ota.h"

#include "gcm.h"

#define MAX_CIPHERTEXT_PACKET_SIZE 32

GCM lea_gcm;

// connected to radio transmitter
CrsfSerial radio_transmitter(Serial1, 400000);

// connected to crsf transmitter aka elrs tx module
CrsfSerial crsf_transmitter(Serial, 400000);

HardwareSerial Serial1(USART1);
HardwareSerial DebugSerial(UART5);

static unsigned long lastSendMspTime = 0;

static uint8_t securityType = 0;
static int securityIsReady = -1;
static unsigned long resetTime = 0;
static bool resetScheduled = false;

void encryptedPacketChannels(uint8_t* buf, uint8_t len);

void sendMspData(uint8_t* payload, uint8_t payload_len) {
  static uint8_t counter = 0;
  Crc8 _crc = Crc8(0xd5);

  uint8_t buf[CRSF_MAX_PACKET_SIZE];
  uint8_t ciphertext[MAX_CIPHERTEXT_PACKET_SIZE];

  int ciphertext_len = lea_gcm.encrypt(payload, payload_len, ciphertext);
  if (ciphertext_len < 0) {
    DebugSerial.println("Encryption failed");
    return;
  }

  DebugSerial.print("Encrypted TX RC Channels: ");
  for (int i = 0; i < ciphertext_len ; i++) {
    DebugSerial.print(ciphertext[i], HEX);
    DebugSerial.print(" ");
  }
  DebugSerial.println();

  buf[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  buf[1] = ciphertext_len + 4; // type + 'CRSF_ADDRESS_FLIGHT_CONTROLLER' + '0' +  ciphertext + crc
  buf[2] = CRSF_FRAMETYPE_MSP_WRITE;
  buf[3] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  buf[4] = 0;
  memcpy(&buf[5], ciphertext, ciphertext_len);
  buf[ciphertext_len+5] = _crc.calc(&buf[2], ciphertext_len + 3);

  crsf_transmitter.write(buf, ciphertext_len + 6);
}

void sendMspData_handshake(uint8_t* payload, uint8_t payload_len) {
  static uint8_t counter = 0;
  Crc8 _crc = Crc8(0xd5);

  uint8_t buf[CRSF_MAX_PACKET_SIZE];

  buf[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  buf[1] = payload_len + 4; // type + 'CRSF_ADDRESS_FLIGHT_CONTROLLER' + '0' +  ciphertext + crc
  buf[2] = CRSF_FRAMETYPE_MSP_WRITE;
  buf[3] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  buf[4] = 0;
  memcpy(&buf[5], payload, payload_len);
  buf[payload_len+5] = _crc.calc(&buf[2], payload_len + 3);

  crsf_transmitter.write(buf, payload_len + 6);
}

void sendCrsfRcChannelsEncrypted(uint8_t* payload, uint8_t payload_len) {
  Crc8 _crc = Crc8(0xd5);

  uint8_t buf[CRSF_MAX_PACKET_SIZE];

  buf[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  buf[1] = payload_len + 2; // type + ciphertext + crc
  buf[2] = CRSF_FRAMETYPE_RC_CHANNELS_ENCRYPTED;
  memcpy(&buf[3], payload, payload_len);
  buf[payload_len+3] = _crc.calc(&buf[2], payload_len + 1);

  crsf_transmitter.write(buf, payload_len + 4);
}

enum HandshakeState {
    WAITING_HELLO = 0,
    SENT_ACK,
    WAITING_PUBKEY,
    SENT_PUBKEY,
    WAITING_BYE,
    COMPLETED
};

static HandshakeState handshakeState = WAITING_HELLO;
static unsigned long lastHandshakeTime = 0;
const unsigned long HANDSHAKE_TIMEOUT = 8000; // 6초 타임아웃

// Message for handshake
const uint8_t MSP_HELLO = 0x01;
const uint8_t MSP_ACK = 0x02;
const uint8_t MSP_PUBKEY = 0x03;
const uint8_t MSP_DATA = 0x04;
const uint8_t MSP_BYE = 0x05;

// TX handshake
void handleTxHandshake(uint8_t* data, uint8_t len) {
  if (len < 2) return;

  uint8_t destAddr = data[0];
  uint8_t srcAddr = data[1];
  uint8_t msgType = data[2];

  DebugSerial.print("TX msgType: ");
  DebugSerial.println(msgType, HEX);

  switch (handshakeState) {
  case WAITING_HELLO:
    if (msgType == MSP_HELLO) {
      DebugSerial.println("--> Handshake() HELLO");
      // Hello 받으면 ACK 전송
      DebugSerial.println("Handshake() ACK -->");
      uint8_t ack[] = {MSP_ACK};
      sendMspData_handshake(ack, sizeof(ack));
      handshakeState = SENT_ACK;
      lastHandshakeTime = millis();
      handshakeState = WAITING_PUBKEY;
      lastHandshakeTime = millis();
    }
    break;
  case WAITING_PUBKEY:
    if (msgType == MSP_PUBKEY) {
      DebugSerial.println("--> Handshake() PUBKEY");
      // pubkey를 받으면, 암호화 하여 데이터(경량암호키)를 보냄
      uint8_t ciphertext[] = {MSP_DATA, 0x01, 0x02, 0x03, 0x04};
      sendMspData_handshake(ciphertext, sizeof(ciphertext));
      handshakeState = SENT_PUBKEY;
      lastHandshakeTime = millis();
      DebugSerial.println("Handshake() SENT_PUBKEY -->");
      handshakeState = WAITING_BYE;
      lastHandshakeTime = millis();
    }
    break;
  case WAITING_BYE:
    if (msgType == MSP_BYE) {
      DebugSerial.println("--> Handshake() BYE");
      handshakeState = COMPLETED;
      lastHandshakeTime = millis();
      DebugSerial.println("Handshake() COMPLETED");
    }
    break;
  default:
    break;
  }
}

void encryptedPacketChannels(uint8_t* buf, uint8_t len) {
  DebugSerial.print("Encrypted RC Channels: ");
  for (int i = 0; i < len + 4; i++) {
    DebugSerial.print(buf[i], HEX);
    DebugSerial.print(" ");
  }
  DebugSerial.println();
}

void generateChannelData(const crsf_channels_t* ch, uint32_t* channelData) {
    channelData[0] = ch->ch0;
    channelData[1] = ch->ch1;
    channelData[2] = ch->ch2;
    channelData[3] = ch->ch3;
    channelData[4] = ch->ch4;
    channelData[5] = ch->ch5;
    channelData[6] = ch->ch6;
    channelData[7] = ch->ch7;
    channelData[8] = ch->ch8;
    channelData[9] = ch->ch9;
    channelData[10] = ch->ch10;
    channelData[11] = ch->ch11;
    channelData[12] = ch->ch12;
    channelData[13] = ch->ch13;
    channelData[14] = ch->ch14;
    channelData[15] = ch->ch15;
}

void encryptedChannels(const crsf_channels_t* src, crsf_channels_encrypted_t* dest, bool isHighAux) {
    uint8_t buf[CRSF_MAX_PACKET_SIZE];
    uint8_t channelData_ch5_ch12;
    uint8_t ciphertext[MAX_CIPHERTEXT_PACKET_SIZE];

    // Initialize the encrypted channels structure
    dest->packetType = 0;  // PACKET_TYPE_RC_DATA
    dest->free = 0;        // Reserved bits set to 0
    dest->isHighAux = isHighAux ? 1 : 0;
    dest->ch4 = (src->ch4 > CRSF_CHANNEL_VALUE_MID) ? 1 : 0;  // AUX1 as binary value

    // Pack first 4 channels (CH0-CH3)
    uint32_t channelData[4];
    channelData[0] = src->ch0;
    channelData[1] = src->ch1;
    channelData[2] = src->ch2;
    channelData[3] = src->ch3;

    OTA_Channels_4x10 tempChannels;
    PackUInt11ToChannels4x10(channelData, &tempChannels);
    memcpy(buf, tempChannels.raw, sizeof(tempChannels));

    PackUInt11ToChannels4x2(src, &channelData_ch5_ch12, isHighAux);
    memcpy(buf + sizeof(tempChannels), &channelData_ch5_ch12, sizeof(channelData_ch5_ch12));

    // Encrypt the channel data
    uint8_t buf_len = sizeof(tempChannels) + sizeof(channelData_ch5_ch12);
    int ciphertext_len = lea_gcm.encrypt(buf, buf_len, ciphertext);
    if (ciphertext_len < 0) {
        DebugSerial.println("Encryption failed");
        return;
    }

    memcpy(dest->raw, ciphertext, ciphertext_len);
}

void printCrsfChannels(crsf_channels_t* src) {
    static uint32_t channelData[CRSF_NUM_CHANNELS] = {0};

    OTA_Channels_4x10 tempChannels;
    channelData[0] = src->ch0;
    channelData[1] = src->ch1;
    channelData[2] = src->ch2;
    channelData[3] = src->ch3;
    PackUInt11ToChannels4x10(channelData, &tempChannels);
    UnpackChannels4x10ToUInt11(&tempChannels, channelData);
    channelData[4] = src->ch4 > CRSF_CHANNEL_VALUE_MID ? CRSF_CHANNEL_VALUE_2000 : CRSF_CHANNEL_VALUE_1000;

    // Extract CH5-CH12 from the last 1 bytes
    uint8_t channelData_ch5_ch12;
    PackUInt11ToChannels4x2(src, &channelData_ch5_ch12, false);
    UnpackChannels4x2ToUInt11(channelData_ch5_ch12, channelData, false);

    for (int i = 0; i < 8; i++) {
        DebugSerial.print(channelData[i]);
        DebugSerial.print(" ");
    }
    DebugSerial.println();
}


void to_crsf_transmitter(const uint8_t* buf, uint8_t len) {
  static uint8_t counter = 0;
  const crsf_header_t *hdr = (crsf_header_t *)buf;

  RC_Channels_t rc;
  crsf_channels_encrypted_t ch_encrypted;

  uint32_t ChannelData[CRSF_NUM_CHANNELS] = {0};
  uint32_t UnpackChannelData[CRSF_NUM_CHANNELS] = {0};

  if (hdr->type == CRSF_FRAMETYPE_PARAMETER_WRITE) {
      if (buf[3] == 0xEE && buf[4] == 0xEF) { // 0xEE destination CRSF_TRANSMITTER, 0xEF source ?
        if (buf[5] == 0x01) { // 0x01 is parameter index for rc security type
          // if securityType changed, write to EEPROM and update securityType
          if (securityType != buf[6]) {
            securityType = buf[6];
         	  EEPROM.write(0x00, securityType); // 0: Off, 1: LEA-GCM, 2: ASCON
            DebugSerial.print("Set Security Type: ");
            DebugSerial.println(securityType);
          
            // Reset the system after 3 seconds, When new security type is set
            resetTime = millis() + 3000;
            resetScheduled = true;
            DebugSerial.println("System will reset in 3 seconds...");
          }
        }
      }
  }
 
  if (hdr->type == CRSF_FRAMETYPE_RC_CHANNELS_PACKED && securityType > 0 && securityIsReady == 0) {
     if (handshakeState == COMPLETED) {
      generateChannelData((crsf_channels_t *)&hdr->data, ChannelData);
      // Packing and Unpacking for test
      // PackUInt11ToChannels4x10(&ChannelData[0], &rc.chLow);
      // PackUInt11ToChannels4x10(&ChannelData[4], &rc.chHigh);
      // UnpackChannels4x10ToUInt11(&rc.chLow, &UnpackChannelData[0]);
      // UnpackChannels4x10ToUInt11(&rc.chHigh, &UnpackChannelData[4]);

      // DebugSerial.print("TX RC Security Module Channels: ");
      encryptedChannels((crsf_channels_t *)&hdr->data, &ch_encrypted, false);

      // DebugSerial.print("Encrypted TX RC Channels: ");
      // for (int i = 0; i < 10; i++) {
      //   DebugSerial.print(ch_encrypted.raw[i], HEX);
      //   DebugSerial.print(" ");
      // }
      // DebugSerial.println();

      sendCrsfRcChannelsEncrypted((uint8_t*)&ch_encrypted, 11); // 1 packetType + 10 ciphertext
    }
  } else {
    crsf_transmitter.write(buf, len);
  }
  Serial.flush();
  counter = (counter + 1) % 256;
}

void to_radio_transmitter(const uint8_t* buf, uint8_t len) {
  const crsf_header_t *hdr = (crsf_header_t *)buf;

  if (hdr->type == CRSF_FRAMETYPE_MSP_WRITE) {
    // __BKPT();
    if (hdr->data[0] == 0xEA && hdr->data[1] == 0) {
      handleTxHandshake((uint8_t*)(hdr->data), hdr->frame_size - 2);
      // DebugSerial.print("GOT MSP_WRITE ");
      // DebugSerial.print(len);
      // DebugSerial.print(" ");
      // DebugSerial.print(buf[len-2], HEX);
      // DebugSerial.print(" ");
      // DebugSerial.print(buf[len-1], HEX);
      // DebugSerial.print(" CRSF: ");
      // DebugSerial.print(hdr->device_addr, HEX);
      // DebugSerial.print(" ");
      // DebugSerial.print(hdr->frame_size);
      // DebugSerial.print(" ");
      // DebugSerial.print(hdr->type, HEX);
      // DebugSerial.print(" ");
      // DebugSerial.print(hdr->data[0], HEX); // dest
      // DebugSerial.print(" ");
      // DebugSerial.print(hdr->data[1], HEX); // src
      // DebugSerial.print(" ");
      // DebugSerial.print(hdr->data[2], HEX); // MSP_*
      // DebugSerial.print(" ");
      // DebugSerial.println(hdr->data[3], HEX);
      return;
    }
  }

  if ( hdr->type == CRSF_FRAMETYPE_PARAMETER_WRITE
    || hdr->type == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY
    || hdr->type == CRSF_FRAMETYPE_DEVICE_INFO
    // || hdr->type == CRSF_FRAMETYPE_LINK_STATISTICS // The LINK_STATISTICS Frame occour Error to EdgeTX
  ) {
    radio_transmitter.queueTxBuffer(buf, len);
  }
}

void setup() {
  SystemClock_Config();

  // Radio Transmitter: UART1 PA9 TX, PA10 RX
  Serial1.setTx(PA_9);
  Serial1.setRx(PA_10);

  // RF Module: qUART4
  Serial.setTx(PC_10);
  Serial.setRx(PC_11);

  // Debug: UART5
  DebugSerial.setTx(PC_12);
  DebugSerial.setRx(PD_2);
  DebugSerial.begin(420000);

  radio_transmitter.begin();
  radio_transmitter.onForward = &to_crsf_transmitter;

  crsf_transmitter.begin();
  crsf_transmitter.onForward = &to_radio_transmitter;

  DebugSerial.println("Starting TX RC Module ...");
  DebugSerial.print("Security Type: ");
  DebugSerial.println(securityType);
  securityType = EEPROM.read(0x00);
  securityIsReady = lea_gcm.init();

  // SKIP handshake
  handshakeState = COMPLETED;
}

void loop() {
  radio_transmitter.loop();
  crsf_transmitter.loop();

  // 비동기 리셋 체크
  if (resetScheduled && millis() >= resetTime) {
    DebugSerial.println("Executing scheduled reset now...");
    NVIC_SystemReset();
  }

  return; // SKIP handshake

  // handshake 타임아웃 처리
  if (handshakeState != WAITING_HELLO && handshakeState != COMPLETED) {
    if (millis() - lastHandshakeTime > HANDSHAKE_TIMEOUT) {
      handshakeState = WAITING_HELLO;
      DebugSerial.println("Handshake timeout");
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}
