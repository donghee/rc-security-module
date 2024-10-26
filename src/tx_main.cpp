#include <Arduino.h>
#include "crc8.h"
#include "CrsfSerial.h"

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

void encryptedPacketChannels(uint8_t* buf, uint8_t len) {
  DebugSerial.print("Encrypted RC Channels: ");
  for (int i = 0; i < len + 4; i++) {
    DebugSerial.print(buf[i], HEX);
    DebugSerial.print(" ");
  }
  DebugSerial.println();
}

void packetChannels() {
  DebugSerial.print("TX RC Channels: ");
  // for (int i = 0; i < 16; i++) {
  for (int i = 0; i < 8; i++) {
    DebugSerial.print(radio_transmitter.getChannel(i + 1));
    DebugSerial.print(" ");
  }
  DebugSerial.println();
}


void to_crsf_transmitter(const uint8_t* buf, uint8_t len) {
  const crsf_header_t *hdr = (crsf_header_t *)buf;
  crsf_transmitter.write(buf, len);
  Serial.flush();

  if (hdr->type == CRSF_FRAMETYPE_RC_CHANNELS_PACKED) {
    // sendMspData((uint8_t*)(hdr->data), hdr->frame_size - 2); // send msp to flight controller from security module
    sendMspData((uint8_t*)(hdr->data), 11); // 11 bits x 8 channels == 88 bits == 11 bytes
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastSendMspTime;

    // 실행 주기 출력
    DebugSerial.print("TX interval: ");
    DebugSerial.print(timeDiff);
    DebugSerial.println(" ms");

    // 현재 시간을 마지막 실행 시간으로 저장
    lastSendMspTime = currentTime;
  }
  // DebugSerial.print("Radio Transmitter->ELRS TX: ");
  // DebugSerial.println("TX RC Channels: 992 992 173 992 173 173 173 173");
}

void to_radio_transmitter(const uint8_t* buf, uint8_t len) {
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
}

void loop() {
  radio_transmitter.loop();
  crsf_transmitter.loop();
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

