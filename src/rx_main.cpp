#include <Arduino.h>
#include "CrsfSerial.h"

// connected to flight controller
CrsfSerial flight_controller(Serial1, 420000);

// connected to crsf receiver aka elrs rx module
CrsfSerial crsf_receiver(Serial, 420000);

HardwareSerial Serial1(USART1);
HardwareSerial DebugSerial(UART5);

void packetChannels() {
  DebugSerial.print("RC Channels: ");
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
  static int counter = 0;
  flight_controller.write(buf, len);
  Serial1.flush();

  const crsf_header_t *hdr = (crsf_header_t *)buf;

  if (hdr->device_addr == CRSF_ADDRESS_FLIGHT_CONTROLLER)
  {
    if (hdr->type == CRSF_FRAMETYPE_MSP_WRITE) {
      counter++;
      DebugSerial.print(counter);
      DebugSerial.print(" MSP_WRITE: ");
      DebugSerial.print(hdr->type, HEX);
      DebugSerial.print(" ");
      for (int i = 0; i < hdr->frame_size - 2; i++) {
        DebugSerial.print(hdr->data[i], HEX);
        DebugSerial.print(" ");
      }
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

