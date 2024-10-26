#include <Arduino.h>
#include "CrsfSerial.h"

#include "gcm.h"

#define MAX_PLAINTEXT_PACKET_SIZE 32

GCM lea_gcm;
// connected to flight controller
CrsfSerial flight_controller(Serial1, 420000);

// connected to crsf receiver aka elrs rx module
CrsfSerial crsf_receiver(Serial, 420000);

HardwareSerial Serial1(USART1);
HardwareSerial DebugSerial(UART5);
static unsigned long lastRecvMspTime = 0;

void packetChannels() {
  DebugSerial.print("RX RC Channels: ");
  for (int i = 0; i < 16; i++) {
    DebugSerial.print(crsf_receiver.getChannel(i + 1));
    DebugSerial.print(" ");
  }
  DebugSerial.println();
}

void to_crsf_receiver(const uint8_t* buf, uint8_t len) {
  crsf_receiver.write(buf, len);
  Serial.flush();
}

void to_flight_controller(const uint8_t* buf, uint8_t len) {
  int plaintext_len = 0;
  uint8_t plaintext[MAX_PLAINTEXT_PACKET_SIZE];
  uint8_t _fcBuf[CRSF_MAX_PACKET_SIZE];
  int _channels[CRSF_NUM_CHANNELS];
  Crc8 _crc = Crc8(0xd5);

  static int counter = 0;
  // flight_controller.write(buf, len);
  // Serial1.flush();

  const crsf_header_t *hdr = (crsf_header_t *)buf;

  if (hdr->device_addr == CRSF_ADDRESS_FLIGHT_CONTROLLER)
  {
    if (hdr->type == CRSF_FRAMETYPE_MSP_WRITE) {
      // TODO: why data[2]? should be data[3]?
      plaintext_len = lea_gcm.decrypt(&hdr->data[2], hdr->frame_size - 4, plaintext);
      if (plaintext_len < 0) {
        DebugSerial.println("Decryption failed");
        return;
      }

      unsigned long currentTime = millis();
      unsigned long timeDiff = currentTime - lastRecvMspTime;

      // 실행 주기 출력: TODO RX interval up to 7.5ms
      DebugSerial.print("RX interval: ");
      DebugSerial.print(timeDiff);
      DebugSerial.println(" ms");

      // 현재 시간을 마지막 실행 시간으로 저장
      lastRecvMspTime = currentTime;

      // DebugSerial.print("Decrypted: ");
      // for (int i = 0; i < plaintext_len; i++) {
      //   DebugSerial.print(plaintext[i], HEX);
      //   DebugSerial.print(" ");
      // }
      // DebugSerial.println();


      // _fcBuf[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
      // _fcBuf[1] = plaintext_len + 2; // type + payload + crc
      // _fcBuf[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
      // memcpy(&_fcBuf[3], plaintext, plaintext_len);
      // _fcBuf[plaintext_len+3] = _crc.calc(&_fcBuf[2], plaintext_len + 1);
      // TODO write rc channels to fc
      // flight_controller.queuePacket(CRSF_ADDRESS_FLIGHT_CONTROLLER, CRSF_FRAMETYPE_RC_CHANNELS_PACKED, plaintext, plaintext_len);

      crsf_channels_t *ch = (crsf_channels_t *)&plaintext;
      _channels[0] = ch->ch0;
      _channels[1] = ch->ch1;
      _channels[2] = ch->ch2;
      _channels[3] = ch->ch3;
      _channels[4] = ch->ch4;
      _channels[5] = ch->ch5;
      _channels[6] = ch->ch6;
      _channels[7] = ch->ch7;

      for (unsigned int i=0; i<CRSF_NUM_CHANNELS; ++i)
        _channels[i] = map(_channels[i], CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);

     DebugSerial.print("RX RC Channels: ");
     DebugSerial.print(_channels[0]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[1]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[2]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[3]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[4]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[5]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[6]);
     DebugSerial.print(" ");
     DebugSerial.print(_channels[7]);
     DebugSerial.print(" ");
     DebugSerial.println();
    }
  }
}

void setup() {
  SystemClock_Config();

  // Flight Controller: UART1
  Serial1.setTx(PA_9);
  Serial1.setRx(PA_10);

  // RF Module: UART4
  Serial.setTx(PC_10);
  Serial.setRx(PC_11);

  flight_controller.begin();
  flight_controller.onForward = &to_crsf_receiver;

  crsf_receiver.begin();
  crsf_receiver.onForward = &to_flight_controller;
  // crsf_receiver.onPacketChannels = &packetChannels;

  // Debug: UART5
  DebugSerial.setTx(PC_12);
  DebugSerial.setRx(PD_2);
  DebugSerial.begin(420000);

  lea_gcm.init();
}

void loop() {
  //flight_controller.loop();
  crsf_receiver.loop();
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
