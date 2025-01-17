#include <Arduino.h>
#include "crc8.h"
#include "CrsfSerial.h"
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

enum HandshakeState {
    WAITING_HELLO = 0,
    SENT_ACK,
    WAITING_PUBKEY,
    SENT_DATA,
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
      handshakeState = SENT_DATA;
      lastHandshakeTime = millis();
      DebugSerial.println("Handshake() SENT_DATA -->");
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

void generateChannelData(const crsf_channels_t* ch, uint32_t* ChannelData) {
    ChannelData[0] = ch->ch0;
    ChannelData[1] = ch->ch1;
    ChannelData[2] = ch->ch2;
    ChannelData[3] = ch->ch3;
    ChannelData[4] = ch->ch4;
    ChannelData[5] = ch->ch5;
    ChannelData[6] = ch->ch6;
    ChannelData[7] = ch->ch7;
    ChannelData[8] = ch->ch8;
    ChannelData[9] = ch->ch9;
    ChannelData[10] = ch->ch10;
    ChannelData[11] = ch->ch11;
    ChannelData[12] = ch->ch12;
    ChannelData[13] = ch->ch13;
    ChannelData[14] = ch->ch14;
    ChannelData[15] = ch->ch15;
}

void printChannelData(uint32_t* channelData) {
    for (int i = 0; i < 8; i++) {
        DebugSerial.print(channelData[i]);
        DebugSerial.print(" ");
    }
    DebugSerial.println();
}

void packetChannels() {
  // DebugSerial.print("TX RC Channels: ");
  // // for (int i = 0; i < 16; i++) {
  // for (int i = 0; i < 8; i++) {
  //   DebugSerial.print(radio_transmitter.getChannel(i + 1));
  //   DebugSerial.print(" ");
  // }
  //  DebugSerial.println();
}


void to_crsf_transmitter(const uint8_t* buf, uint8_t len) {
  static uint8_t counter = 0;
  const crsf_header_t *hdr = (crsf_header_t *)buf;

  RC_s rc;
  uint32_t ChannelData[CRSF_NUM_CHANNELS] = {0};
  uint32_t UnpackChannelData[CRSF_NUM_CHANNELS] = {0};

  if (hdr->type == CRSF_FRAMETYPE_RC_CHANNELS_PACKED) {
     if (handshakeState == COMPLETED) {
    // sendMspData((uint8_t*)(hdr->data), hdr->frame_size - 2); // send msp to flight controller from security module
    // sendMspData((uint8_t*)(hdr->data), 11); // 11 bits x 8 channels == 88 bits == 11 bytes
      // sendMspData((uint8_t*)(hdr->data), 10); // 12ms
      
      generateChannelData((crsf_channels_t *)&hdr->data, ChannelData);
      PackUInt11ToChannels4x10(&ChannelData[0], &rc.chLow);
      PackUInt11ToChannels4x10(&ChannelData[4], &rc.chHigh);
      // for unpacking test
      UnpackChannels4x10ToUInt11(&rc.chLow, &UnpackChannelData[0]);
      UnpackChannels4x10ToUInt11(&rc.chHigh, &UnpackChannelData[4]);

      DebugSerial.print("10 Bytes TX RC Channels: "); // TODO: decrease to 7 bytes
      printChannelData(UnpackChannelData);

      sendMspData((uint8_t*)&rc, 10);
    }
    // if (counter % 8 == 0) {
     crsf_transmitter.write(buf, len);
     Serial.flush();
    // }
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastSendMspTime;

    // 실행 주기 출력
    // DebugSerial.print("TX interval: ");
    // DebugSerial.print(timeDiff);
    // DebugSerial.println(" ms");

    // 현재 시간을 마지막 실행 시간으로 저장
    lastSendMspTime = currentTime;
  } else {
    crsf_transmitter.write(buf, len);
    Serial.flush();
  }
  // DebugSerial.print("Radio Transmitter->ELRS TX: ");
  // DebugSerial.println("TX RC Channels: 992 992 173 992 173 173 173 173");
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

  radio_transmitter.write(buf, len);
  Serial1.flush();
  // DebugSerial.print("ELRS TX->Radio Transmitter: ");
}

void setup() {
  SystemClock_Config();

  // RC Controller: UART1
  Serial1.setTx(PA_9);
  Serial1.setRx(PA_10);

  // RF Module: qUART4
  Serial.setTx(PC_10);
  Serial.setRx(PC_11);

  radio_transmitter.begin();
  radio_transmitter.onPacketChannels = &packetChannels;
  radio_transmitter.onForward = &to_crsf_transmitter;

  crsf_transmitter.begin();
  crsf_transmitter.onForward = &to_radio_transmitter;

  // Debug: UART5
  DebugSerial.setTx(PC_12);
  DebugSerial.setRx(PD_2);
  DebugSerial.begin(420000);

  lea_gcm.init();

  // SKIP handshake
  handshakeState = COMPLETED;
}

void loop() {
  radio_transmitter.loop();
  crsf_transmitter.loop();

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
