## 2025-05

sx1280 SPI 통신 테스트 TX/RX

```c++
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

#define GPIO_PIN_DIO1               PG14

#define PIN_SPI_SS            PI0
#define PIN_SPI_MOSI          PB15
#define PIN_SPI_MISO          PB14
#define PIN_SPI_SCK           PI1

#define NRESET  PB5                      //SX128X reset pin
#define RFBUSY  PG13                     //SX128X busy pin

#define LED1 PG9                         //for on board LED, put high for on

uint8_t saveddevice;

bool begin(int8_t pinNSS, int8_t pinNRESET, int8_t pinRFBUSY, uint8_t device);
void checkBusy();
void writeCommand(uint8_t Opcode, uint8_t *buffer, uint16_t size);
bool checkDevice();
void resetDevice();
void printRegisters(uint16_t Start, uint16_t End);
uint32_t getFreqInt();
void setPacketType(uint8_t packettype);
void setRfFrequency(uint32_t frequency, int32_t offset);

const uint16_t REG_RFFrequency23_16 = 0x906;
const uint16_t REG_RFFrequency15_8 = 0x907;
const uint16_t REG_RFFrequency7_0 = 0x908;
const uint8_t RADIO_WRITE_REGISTER = 0x18;
const uint8_t RADIO_READ_REGISTER = 0x19;
const uint8_t RADIO_SET_RFFREQUENCY = 0x86;             //commnad to change frequency
const uint8_t RADIO_SET_PACKETTYPE =  0x8A;             //commnad to set packet mode
const float FREQ_STEP = 198.364;
const uint8_t PACKET_TYPE_LORA = 0x01;

HardwareSerial Serial1(USART1);
HardwareSerial DebugSerial(UART4);

// SPIClass SPI_2(PB_15, PB_14, PI_1);
SPIClass SPI_2(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);

void setup() {
  SystemClock_Config();

  pinMode(LED1, OUTPUT); // LED

  DebugSerial.setTx(PC_10);
  DebugSerial.setRx(PC_11);
  DebugSerial.begin(420000);
  DebugSerial.println("LED Blink");

  SPI_2.begin();
  SPI_2.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  begin(PIN_SPI_SS, NRESET, RFBUSY, 0); // Initialize the SX128x device
}

void loop() {
  // digitalWrite(LED1, HIGH); // LED ON
  // delay(1000);
  // digitalWrite(LED1, LOW); // LED OFF
  // delay(1000);
  // DebugSerial.println("LED Blink");

  // LORA
  uint32_t frequency;
  resetDevice();                             //reset the device
  DebugSerial.println(F("Registers at reset"));   //show the all registers following a reset
  printRegisters(0x0900, 0x09FF);
  DebugSerial.println();
  DebugSerial.println();

  frequency = getFreqInt();                  //read the set frequency following a reset
  DebugSerial.print(F(" Frequency at reset "));
  DebugSerial.print(frequency);
  DebugSerial.println(F("hz"));

  DebugSerial.print(F("Change Frequency to 2445000000hz"));
  setPacketType(PACKET_TYPE_LORA);           //this is needed to ensure frequency change is reflected in register print
  setRfFrequency(2445000000, 0);             //change the frequency to 2445000000hertz

  frequency = getFreqInt();                  //read back the changed frequency
  DebugSerial.println();
  DebugSerial.print(F("      Frequency now "));
  DebugSerial.print(frequency);                   //print the changed frequency, did the write work (allow for rounding errors) ?
  DebugSerial.println(F("hz"));
  DebugSerial.println();
  printRegisters(0x0900, 0x090F);            //show the registers after frequency change
  DebugSerial.println();
  DebugSerial.println();
  delay(5000);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV8;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void readRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
  uint16_t index;
  uint8_t addr_l, addr_h;

  addr_h = address >> 8;
  addr_l = address & 0x00FF;
  checkBusy();

  digitalWrite(PIN_SPI_SS, LOW);
  SPI_2.transfer(RADIO_READ_REGISTER);
  SPI_2.transfer(addr_h);               //MSB
  SPI_2.transfer(addr_l);               //LSB
  SPI_2.transfer(0xFF);
  for (index = 0; index < size; index++)
  {
    *(buffer + index) = SPI_2.transfer(0xFF);
  }

  digitalWrite(PIN_SPI_SS, HIGH);
  checkBusy();
}


uint8_t readRegister(uint16_t address)
{
  uint8_t data;

  readRegisters(address, &data, 1);
  return data;
}


void writeRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
  uint8_t addr_l, addr_h;
  uint8_t i;

  addr_l = address & 0xff;
  addr_h = address >> 8;
  checkBusy();

  digitalWrite(PIN_SPI_SS, LOW);
  SPI_2.transfer(RADIO_WRITE_REGISTER);
  SPI_2.transfer(addr_h);   //MSB
  SPI_2.transfer(addr_l);   //LSB

  for (i = 0; i < size; i++)
  {
    SPI_2.transfer(buffer[i]);
  }

  digitalWrite(PIN_SPI_SS, HIGH);

  checkBusy();
}


void writeRegister(uint16_t address, uint8_t value)
{
  writeRegisters(address, &value, 1 );
}


uint32_t getFreqInt()
{
  //get the current set device frequency, return as long integer
  uint8_t Msb, Mid, Lsb;
  uint32_t uinttemp;
  float floattemp;
  Msb = readRegister(REG_RFFrequency23_16);
  Mid = readRegister(REG_RFFrequency15_8);
  Lsb = readRegister(REG_RFFrequency7_0);
  floattemp = ((Msb * 0x10000ul) + (Mid * 0x100ul) + Lsb);
  floattemp = ((floattemp * FREQ_STEP) / 1000000ul);
  uinttemp = (uint32_t)(floattemp * 1000000);
  return uinttemp;
}


void printRegisters(uint16_t Start, uint16_t End)
{
  //prints the contents of SX128x registers to serial monitor

  uint16_t Loopv1, Loopv2, RegData;

  DebugSerial.print(F("Reg    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F"));
  DebugSerial.println();

  for (Loopv1 = Start; Loopv1 <= End;)           //32 lines
  {
    DebugSerial.print(F("0x"));
    DebugSerial.print((Loopv1), HEX);                 //print the register number
    DebugSerial.print(F("  "));
    for (Loopv2 = 0; Loopv2 <= 15; Loopv2++)
    {
      RegData = readRegister(Loopv1);
      if (RegData < 0x10)
      {
        DebugSerial.print(F("0"));
      }
      DebugSerial.print(RegData, HEX);                //print the register number
      DebugSerial.print(F(" "));
      Loopv1++;
    }
    DebugSerial.println();
  }
}


void setRfFrequency(uint32_t frequency, int32_t offset)
{
  frequency = frequency + offset;
  uint8_t buffer[3];
  uint32_t freqtemp = 0;
  freqtemp = ( uint32_t )( (float) frequency / (float) FREQ_STEP);
  buffer[0] = ( uint8_t )( ( freqtemp >> 16 ) & 0xFF );
  buffer[1] = ( uint8_t )( ( freqtemp >> 8 ) & 0xFF );
  buffer[2] = ( uint8_t )( freqtemp & 0xFF );
  writeCommand(RADIO_SET_RFFREQUENCY, buffer, 3);
  writeCommand(RADIO_SET_RFFREQUENCY, buffer, 3);
}


void checkBusy()
{
  uint8_t busy_timeout_cnt;
  busy_timeout_cnt = 0;

  while (digitalRead(RFBUSY))
  {
    delay(1);
    busy_timeout_cnt++;

    if (busy_timeout_cnt > 10) //wait 10mS for busy to complete
    {
      busy_timeout_cnt = 0;
      DebugSerial.println(F("ERROR - Busy Timeout!"));
      break;
    }
  }
}


void resetDevice()
{
  DebugSerial.println(F("Reset device"));
  delay(10);
  digitalWrite(NRESET, LOW);
  delay(2);
  digitalWrite(NRESET, HIGH);
  delay(25);
  checkBusy();
}



bool begin(int8_t pinNSS, int8_t pinNRESET, int8_t pinRFBUSY, uint8_t device)
{
  saveddevice = device;

  pinMode(pinNSS, OUTPUT);
  digitalWrite(pinNSS, HIGH);
  pinMode(pinNRESET, OUTPUT);
  digitalWrite(pinNRESET, HIGH);
  pinMode(pinRFBUSY, INPUT);

  resetDevice();

  if (checkDevice())
  {
    return true;
  }

  return false;
}


bool checkDevice()
{
  //check there is a device out there, writes a register and reads back

  uint8_t Regdata1, Regdata2;
  Regdata1 = readRegister(0x0908);               //low byte of frequency setting
  writeRegister(0x0908, (Regdata1 + 1));
  Regdata2 = readRegister(0x0908);               //read changed value back
  writeRegister(0x0908, Regdata1);             //restore register to original value

  if (Regdata2 == (Regdata1 + 1))
  {
    return true;
  }
  else
  {
    return false;
  }
}


void writeCommand(uint8_t Opcode, uint8_t *buffer, uint16_t size)
{
  uint8_t index;
  checkBusy();

  digitalWrite(PIN_SPI_SS, LOW);
  SPI_2.transfer(Opcode);

  for (index = 0; index < size; index++)
  {
    SPI_2.transfer(buffer[index]);
    //Serial.println(buffer[index], HEX);
  }
  digitalWrite(PIN_SPI_SS, HIGH);

  checkBusy();
}

void setPacketType(uint8_t packettype)
{
  writeCommand(RADIO_SET_PACKETTYPE, &packettype, 1);
} 
```

## 2025-03-17

Serial1.setHalfDuplex로는 One Wire Serial만 된다, inverter 하드웨어 수정 필요한가?
CrsfSerial에서 write를 바로 안하고 read로 crc확인후 txbufs에 내용이 있으면 write 하도록 수정
작동은 되나 느리다.


## 2025-01-30

tx rc-security-module에 의해 RC 조정기의 파워가 죽는다.
tx rc-security-module의 시리얼 포트를 half duplex 로 수정 필요

```c++
#include <HardwareSerial.h>

void setup() {
  // 시리얼 초기화 (보레이트만 설정)
  Serial1.begin(9600);
  
  // USART1의 CR3 레지스터에 HDSEL 비트 설정 (Half Duplex 활성화)
  USART1->CR3 |= USART_CR3_HDSEL;
  
  // GPIO 핀을 Open-Drain 모드로 재설정 (PA9)
  pinMode(PA9, OUTPUT_OPEN_DRAIN);
}

void loop() {
  // 데이터 송신
  Serial1.write("Hello");
  delay(100);
  
  // 데이터 수신 대기
  while (Serial1.available()) {
    char c = Serial1.read();
    // 처리 로직
  }
}
```

```c++
#include <HardwareSerial.h>

void setup() {
  // 시리얼 초기화
  Serial1.begin(9600);
  
  // HAL 라이브러리를 통해 Half Duplex 모드 설정
  Serial1.getHandle()->Init.HalfDuplexMode = UART_HALF_DUPLEX_ENABLE;
  HAL_UART_Init(Serial1.getHandle());
}

void loop() {
  // 송신 및 수신 코드
}
```
