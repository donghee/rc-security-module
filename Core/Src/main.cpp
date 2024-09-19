/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "INA219.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SRAM_DEVICE_ADDR ((uint32_t) 0x60000000)
#define UART_BUF_SIZE 8
#define BUFFER_SIZE 256
#define TAMPER_FORCE 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
INA219_t ina219;

uint8_t pinstat;
uint16_t buff_read[256];
uint8_t uart_buf1[UART_BUF_SIZE];	//uart1 receive buf
uint8_t uart_buf4[UART_BUF_SIZE];	//uart4 receive buf
uint8_t uart_buf5[UART_BUF_SIZE];	//uart5 receive buf
uint8_t comp_flas;
uint8_t print;
uint8_t pw_input;

Cmd_buf cmd_buf;

char * cmd_read = "read";
char * cmd_write = "write";
char * cmd_write_rand = "write_rand";
char * cmd_cmp = "compare";
char * cmd_write_rand_comp = "write_rand_comp";
char * cmd_cmp_rand = "compare_rand";
char * cmd_print = "print";

char * cmd_sram_help = "sram_help";
char * cmd_help = "help";
char * cmd_exit = "exit";


char * cmd_i2c = "i2c_test";
char * cmd_adc = "adc_test";
char * cmd_gpio = "sram_stat";

char * user_id = "root";
char * user_pw = "root";

uint16_t buff_write[32] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
uint16_t buff_rand_comp[10000];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
#if 0
int __io_putchar(uint8_t ch){
	HAL_UART_Transmit(&huart4, &ch, 1, 1000);
    return ch;
}
#endif
void id_pw(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t adc1_value;
uint32_t adc3_value;


void dprintf1(const char* format, ...)
{
	char buf[128] = {0x00, };
	int len = 0;

	va_list ap;

	va_start(ap, format);
	len = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 1000);
}

void dprintf4(const char* format, ...)
{
	char buf[128] = {0x00, };
	int len = 0;

	va_list ap;

	va_start(ap, format);
	len = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	HAL_UART_Transmit(&huart4, (uint8_t*)buf, len, 1000);
}

void dprintf5(const char* format, ...)
{
	char buf[128] = {0x00, };
	int len = 0;

	va_list ap;

	va_start(ap, format);
	len = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	HAL_UART_Transmit(&huart5, (uint8_t*)buf, len, 1000);
}

void INA219_proc(void)
{
	uint16_t vbus, vshunt, current;

	vbus = INA219_ReadBusVoltage(&ina219);
	vshunt = INA219_ReadShuntVolage(&ina219);
	current = INA219_ReadCurrent(&ina219);
	dprintf4("vbus:%d\r\nvshunt:%d\r\ncurrent:%d\r\n \r\n",vbus,vshunt,current);
}

void ADC_proc(void)
{
    //HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R,4095);
    //HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

    //HAL_GPIO_WritePin(GPIOI, GPIO_PIN_9, 0);


	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	adc1_value = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	HAL_ADC_Start(&hadc3);
	HAL_ADC_PollForConversion(&hadc3, HAL_MAX_DELAY);
	adc3_value = HAL_ADC_GetValue(&hadc3);
	HAL_ADC_Stop(&hadc3);

	dprintf4("ADC1: %ld\r\nAD3: %ld\r\n",adc1_value,adc3_value);
	HAL_Delay(100);
}


void sd_write(uint32_t start, uint32_t end)
{
	uint32_t i;
	uint16_t data = 0;
	uint32_t buf_size = end-start;

	dprintf4("\r\nwriting start");
	if(print == 1) dprintf4("\r\n");

	for(i=0;i<buf_size;i++) {
		buff_write[0] = data;
		HAL_SRAM_Write_16b(&hsram1, (uint32_t *) SRAM_DEVICE_ADDR+start+i, buff_write, 1);
		data++;
		if(data > 65534) data = 0;
		if(print == 1) dprintf4("%x ", buff_write[0]);
	} if(print == 1) dprintf4("\r\n");
}

void sd_write_rand_compare(uint32_t start, uint32_t end)
{
	uint32_t i;
	uint16_t data = 0;
	uint32_t buf_size = end-start;

	dprintf4("\r\nwriting start");
	if(print == 1) dprintf4("\r\n");

	for(i=0;i<buf_size;i++) {
		buff_write[0] = rand();
		HAL_SRAM_Write_16b(&hsram1, (uint32_t *) SRAM_DEVICE_ADDR+start+i, buff_write, 1);
		buff_rand_comp[start+i] = buff_write[0];
		data++;
		if(data > 65534) data = 0;

		if(print == 1) dprintf4("%x ", buff_write[0]);
	} if(print == 1) dprintf4("\r\n");
}

void sd_write_rand(uint32_t start, uint32_t end)
{
	uint32_t i;
	uint32_t buf_size = end-start;

	dprintf4("\r\nrandom writing start");
	if(print == 1) dprintf4("\r\n");

	for(i=0;i<buf_size;i++) {
		buff_write[0] = rand();
		HAL_SRAM_Write_16b(&hsram1, (uint32_t *) SRAM_DEVICE_ADDR+start+i, buff_write, 1);

		if(print == 1) dprintf4("%x ", buff_write[0]);
	} if(print == 1) dprintf4("\r\n");
}

void sd_read(uint32_t start, uint32_t end)
{
	uint32_t i;
	uint32_t buf_size = end-start;

	dprintf4("\r\nread data : \r\n");

	for(i=0; i<(buf_size); i++) {
		HAL_SRAM_Read_16b(&hsram1, (uint32_t *) SRAM_DEVICE_ADDR+start+i, buff_read, 1);

		dprintf4("%x ", buff_read[0]);
	} dprintf4("\r\n");
}
void sd_comp_rand(uint32_t start, uint32_t end)
{
	uint32_t i;
	uint16_t data = start;
	uint32_t same = 0;
	uint32_t diff = 0;
	float result;
	uint32_t buf_size = end-start;
	//uint8_t back[3] = {0x08, 0x20, 0x08};

	dprintf4("\r\ncomparing start");

	for(i=0; i<buf_size; i++) {
		HAL_SRAM_Read_16b(&hsram1, (uint32_t *) SRAM_DEVICE_ADDR+start+i, buff_read, 1);

		if(buff_read[0] == buff_rand_comp[start+i]) {
			same++;
		} else {
			diff++;
		}
		data++;
		if(data > 65534) {
			data = 0;
		}
	} dprintf4("\r\n");

	result = ((float)diff/(float)i)*100;
	dprintf4("compare result : same = %d, diff = %d\r\n", same, diff);
	dprintf4("Erasure rate : %0.2f%%\r\n\n", result);
}

void sd_comp(uint32_t start, uint32_t end)
{
	uint32_t i;
	uint16_t data = start;
	uint32_t same = 0;
	uint32_t diff = 0;
	float result;
	uint32_t buf_size = end-start;
	//uint8_t back[3] = {0x08, 0x20, 0x08};

	dprintf4("\r\ncomparing start");

	for(i=0; i<buf_size; i++) {
		HAL_SRAM_Read_16b(&hsram1, (uint32_t *) SRAM_DEVICE_ADDR+start+i, buff_read, 1);
		if(buff_read[0] == data) {
			same++;
		} else {
			diff++;
		}
		data++;
		if(data > 65534) {
			data = 0;
		}
		/*for(int j=0;j<2000;j++) end++;
		if((data%200000) == 0) {
			dprintf4(".");
		}*/
	} dprintf4("\r\n");

	result = ((float)diff/(float)i)*100;
	dprintf4("compare result : same = %d, diff = %d\r\n", same, diff);
	dprintf4("Erasure rate : %0.2f%%\r\n\n", result);
}


void uart4_cmd(void)
{
	if(comp_flas == 1) {
		if(cmd_buf.count > 0) {
			char buf[32];
			//int num[32];
			int num_count = 0;
			char *token;
			int i = 0;

			memset(buf, 0, 32);
			memcpy(buf, cmd_buf.cmd, cmd_buf.count);

			token = strtok(buf, " ");
			token = strtok(NULL, " ");

			while(token !=NULL) {
				cmd_buf.number[i] = atoi(token);

				token = strtok(NULL, " ");
				i++;
				num_count++;
			}


			if(!strcmp(buf, cmd_read)) {
				if(num_count == 2 && (cmd_buf.number[1]-cmd_buf.number[0]) <= 1000) {
					sd_read(cmd_buf.number[0], cmd_buf.number[1]);

					dprintf4("\r\nread SDcard from %d to %d\r\n\n", cmd_buf.number[0], cmd_buf.number[1]);
				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_write)) {
				if(num_count == 2 && cmd_buf.number[1] <= 100000) {
					sd_write(cmd_buf.number[0], cmd_buf.number[1]);

					dprintf4("\r\nwrite SDcard from %d to %d\r\n\n", cmd_buf.number[0], cmd_buf.number[1]);
				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_write_rand)) {
				if(num_count == 2  && cmd_buf.number[1] <= 100000) {
					sd_write_rand(cmd_buf.number[0], cmd_buf.number[1]);

					dprintf4("\r\nwrite SDcard from %d to %d\r\n\n", cmd_buf.number[0], cmd_buf.number[1]);
				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_cmp)) {
				if(num_count == 2  && cmd_buf.number[1] <= 100000) {
					sd_comp(cmd_buf.number[0], cmd_buf.number[1]);

				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_print)) {
				if(num_count == 1) {
					print = cmd_buf.number[0];
				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_write_rand_comp)) {
				if(num_count == 2  && cmd_buf.number[1] <= 10000) {
					sd_write_rand_compare(cmd_buf.number[0], cmd_buf.number[1]);

					dprintf4("\r\nwrite SDcard from %d to %d\r\n\n", cmd_buf.number[0], cmd_buf.number[1]);
				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_cmp_rand)) {
				if(num_count == 2  && cmd_buf.number[1] <= 10000) {
					sd_comp_rand(cmd_buf.number[0], cmd_buf.number[1]);

				} else {
					dprintf4("Enter sram_help to see commands\r\n");
				}

			} else if (!strcmp(buf, cmd_i2c)) {
				dprintf4("Start_I2C_TEST!\r\n\n");
				INA219_proc();

			} else if (!strcmp(buf, cmd_adc)) {
				dprintf4("Start_ADC_TEST!\r\n\n");
				ADC_proc();

			} else if (!strcmp(buf, cmd_gpio)) {
				dprintf4("SRAM_STAT_TEST!\r\n\n");
				if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4))
					 dprintf4("SRAM Voltage is OK!\r\n\n");
				else dprintf4("SRAM Voltage is NOK!\r\n\n");


			}  else if (!strcmp(buf, cmd_exit)) {
				dprintf4("LOGOUT!\r\n\n");
				HAL_Delay(500);
				id_pw();

			} else if (!strcmp(buf, cmd_help)) {
				dprintf4("\r\n**********************************************************\r\n");
				dprintf4("\r*                          HELP                          *\r\n");
				dprintf4("\r**********************************************************\r\n\n");
				dprintf4(" sram_help \t\t To see sram command\r\n");
				dprintf4(" i2c_test \t\t To test INA219 \r\n");
				dprintf4(" adc_test \t\t To test PUF1/PUF2 \r\n");
				dprintf4(" exit \r\n");
				dprintf4("\r\n");
			}  else if (!strcmp(buf, cmd_sram_help)) {
				dprintf4("\r\n**********************************************************\r\n");
				dprintf4("\r*                        SRAM HELP                        *\r\n");
				dprintf4("\r**********************************************************\r\n\n");
				dprintf4(" read \t\t [end addreess]-[start address] < 1000 \r\n");
				dprintf4(" write \t\t [start address] [end addreess] 0~100000 \r\n");
				dprintf4(" write_rand \t [start address] [end addreess] 0~100000 \r\n");
				dprintf4(" compare \t [start address] [end addreess] 0~100000 \r\n");
				dprintf4(" write_rand_comp [start address] [end addreess] 0~10000 \r\n");
				dprintf4(" compare_rand \t [start address] [end addreess] 0~10000 \r\n");
				dprintf4(" print \t\t [1 or 0]\r\n");
				dprintf4("\r\n");
			} else {
				dprintf4("Enter help to see commands\r\n");
			}

			comp_flas=0;
			dprintf4("Fisys~ $ ");
		} else {
			dprintf4("Fisys~ $ ");
			comp_flas=0;
		}
	}
}


void id_pw(void)
{
	char id_buf[32];
	char pw_buf[32];
	int auth = 1;
	int count = 0;

	comp_flas = 0;

	while(auth) {
		memset(id_buf, 0, 32);
		memset(pw_buf, 0, 32);
		dprintf4("ID : ");
		while(auth) {
			if(comp_flas == 1) {
				if(cmd_buf.count == 0) {
					dprintf4("ID : ");
					comp_flas=0;
				} else if(cmd_buf.count > 0) {
					memcpy(id_buf, cmd_buf.cmd, cmd_buf.count);
					comp_flas=0;
					auth=0;
				}
			}
		} auth=1;

		pw_input = 1;

		dprintf4("PW : ");
		while(auth) {
			if(comp_flas == 1) {
				if(cmd_buf.count == 0) {
					dprintf4("PW : ");
					comp_flas=0;
				} else if(cmd_buf.count > 0) {
					memcpy(pw_buf, cmd_buf.cmd, cmd_buf.count);
					comp_flas=0;
					auth=0;
				}
			}
		} auth=1;

		pw_input = 0;

		if (!strcmp(id_buf, user_id) && !strcmp(pw_buf, user_pw)) {
			auth = 0;
			dprintf4("\r\n\nWelcome!\r\n\n");
			HAL_Delay(500);

		} else {
			if(count > 2) {
				dprintf4("Too many tries...\r\n");
				dprintf4("Device locked!\r\n");
				HAL_UART_DeInit(&huart4);

				while(1) {
					HAL_Delay(1000);
				}
			} else {
				dprintf4("Wrong ID or PASSWORD. Please check again!\r\n\n");
				count++;
			}

		}
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_FMC_Init();
  MX_UART4_Init();
  MX_UART5_Init();
  MX_USART1_UART_Init();
  MX_ADC3_Init();
  MX_DAC_Init();
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
  uint8_t buffer1[256];
  uint8_t buffer5[256];
  int count1 = 10;
//  int count4 = 0;
  int count5 = 20;

  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);	//SRAM ERASE PD6 should be set HIGH

  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4))
  	   dprintf4("SRAM Voltage is OK!\r\n");
  else dprintf4("SRAM Voltage is NOK!\r\n");

  while(!INA219_Init(&ina219, &hi2c3, INA219_ADDRESS))
  {

  }

  dprintf4("\r\n\n===DSM-ST START!===\n\n\r");

  /*========== UART1 TEST ===========*/
  sprintf((char *)buffer1, "Test UART1_Tx! %d\r\n", count1);
  dprintf1("%s \r\n", buffer1);
  //  HAL_UART_Transmit(&huart1, buffer1, strlen((char *)buffer1), 100);
  HAL_Delay(100);

  /*========== UART5 TEST ===========*/
  sprintf((char *)buffer5, "Test UART5 Tx! %d\r\n", count5);
  dprintf5("%s \r\n", buffer5);
//  HAL_UART_Transmit(&huart5, buffer5, strlen((char *)buffer5), 100);
  HAL_Delay(100);


  HAL_UART_Receive_IT(&huart1, uart_buf1, 1);
  HAL_UART_Receive_IT(&huart4, uart_buf4, 1);
  HAL_UART_Receive_IT(&huart5, uart_buf5, 1);

  id_pw();
  dprintf4("Fisys~ $ ");




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if 0
	  /*========== UART4 TEST ===========*/
	  printf("Hello Fisys %d\r\n", count4);
	  count4++;
	  if(count4 == 256)	count4 = 0;

  	  HAL_Delay(100);

	  /*========== UART1 TEST ===========*/
  	  sprintf((char *)buffer1, "Test UART1_Loop! %d\r\n", ++count1);
	  HAL_UART_Transmit(&huart1, buffer1, strlen((char *)buffer1), 100);
	  if(count1 == 256)	count1 = 0;
	  HAL_Delay(100);

	  /*========== UART5 TEST ===========*/
	  sprintf((char *)buffer5, "Test UART5 Loop! %d\r\n", ++count5);
	  HAL_UART_Transmit(&huart5, buffer5, strlen((char *)buffer5), 100);
	  if(count5 == 256)	count5 = 0;
	  HAL_Delay(100);

	  /*===== ADC1/ADC3 TEST ===========*/
	  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R,4095);
	  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

	  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_9, 0);

	  HAL_ADC_Start(&hadc1);
	  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	  adc1_value = HAL_ADC_GetValue(&hadc1);
	  HAL_ADC_Stop(&hadc1);

	  HAL_ADC_Start(&hadc3);
	  HAL_ADC_PollForConversion(&hadc3, HAL_MAX_DELAY);
	  adc3_value = HAL_ADC_GetValue(&hadc3);
	  HAL_ADC_Stop(&hadc3);

	  printf("ADC1: %ld\r\nAD3: %ld\r\n",adc1_value,adc3_value);
	  HAL_Delay(1000);

	  /*========== I2C(INA219) VCC/CURENT CAL TEST ===========*/
	  INA219_proc();
	  HAL_Delay(1000);
#endif
	  uart4_cmd();	//cmd input parsing
	  HAL_Delay(1);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
