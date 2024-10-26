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

  static int counter = 0;
  // flight_controller.write(buf, len);
  // Serial1.flush();

  const crsf_header_t *hdr = (crsf_header_t *)buf;

  if (hdr->device_addr == CRSF_ADDRESS_FLIGHT_CONTROLLER)
  {
    // if (hdr->type == CRSF_FRAMETYPE_MSP_WRITE) {
    //
    //   counter++;
    //   DebugSerial.print(counter);
    //   DebugSerial.print(" MSP_WRITE: ");
    //   DebugSerial.print(hdr->type, HEX);
    //   DebugSerial.print(" ");
    //   for (int i = 0; i < hdr->frame_size - 2; i++) {
    //     DebugSerial.print(hdr->data[i], HEX);
    //     DebugSerial.print(" ");
    //   }
    //   DebugSerial.println();
    // }

    // Decrypt
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
     crsf_channels_t *ch = (crsf_channels_t *)&plaintext;
     int ch0 = ch->ch0;
     int ch1 = ch->ch1;
     int ch2 = ch->ch2;
     int ch3 = ch->ch3;
     int ch4 = ch->ch4;
     int ch5 = ch->ch5;
     int ch6 = ch->ch6;
     int ch7 = ch->ch7;

     DebugSerial.print("RX RC Channels: ");
     DebugSerial.print(ch0);
     DebugSerial.print(" ");
     DebugSerial.print(ch1);
     DebugSerial.print(" ");
     DebugSerial.print(ch2);
     DebugSerial.print(" ");
     DebugSerial.print(ch3);
     DebugSerial.print(" ");
     DebugSerial.print(ch4);
     DebugSerial.print(" ");
     DebugSerial.print(ch5);
     DebugSerial.print(" ");
     DebugSerial.print(ch6);
     DebugSerial.print(" ");
     DebugSerial.print(ch7);
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

