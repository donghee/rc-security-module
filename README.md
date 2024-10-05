# RC Security Module

STM32F479IIHx

## Connections

```mermaid
flowchart TD;
    A[RC Controller] -- CRSF/UART --> B[RC Security Module]
    B -- CRSF/UART --> C[RF Transmitter]
    D[RF Receiver] -- CRSF/UART --> E[RC Security Module]
    E -- CRSF/UART --> F[Flight Controller]

    C[RF Transmitter] <-. OTA/📡↔️📡 .-> D[RF Receiver]
```

### Debug Connector Pin Out

| Pin Number | Description |
|------------|-------------|
| 1          | VCC         |
| 2          | SWDIO       |
| 3          | SWCLK       |
| 4          | GND         |
| 5          | NRST        |
| 6          | SWO         |

### Uarts Connector Pin Out

| Pin Number | Description |
|------------|-------------|
| 1          | VIN 5~12V   |
| 2          | UART1 TX    |
| 3          | UART1 RX    |
| 4          | UART4 TX    |
| 5          | UART4 RX    |
| 6          | UART5 TX    |
| 7          | UART5 RX    |
| 8          | GND         |

### UART Pinmap

STM32F479IIHx UART Pinmap

```
// PA10     ------> USART1_RX
// PA9      ------> USART1_TX

// PC11     ------> UART4_RX
// PC10     ------> UART4_TX

// PC2      ------> UART5_RX
// PC12     ------> UART5_RX
```

### CRSF protocol

https://github.com/crsf-wg/crsf/wiki

```mermaid
sequenceDiagram
    participant TX as Transmitter
    participant RX as Receiver
    participant FC as Flight Controller

    Note over TX,FC: CRSF Frame Structure
    Note over TX,FC: [Device Address][Frame Length][Type][Payload][CRC]

    TX->>RX: RC Channels Data
    TX->>RX: Link Statistics
    RX->>FC: RC Channels Data
    RX->>FC: Link Statistics
    TX->>RX: Telemetry Data
    RX->>FC: Telemetry Data
    FC->>RX: Telemetry Data
    RX->>TX: Telemetry Data

    Note over TX,FC: Bidirectional Communication
```

이 다이어그램은 CRSF 프로토콜의 기본 구조와 데이터 흐름을 보여줍니다:
- 프레임 구조: Device Address, Frame Length, Type, Payload, CRC
- 송신기(TX)에서 수신기(RX)로 RC 채널 데이터와 링크 통계 전송
- 수신기(RX)에서 비행 컨트롤러(FC)로 RC 채널 데이터와 링크 통계 전달
- 비행 컨트롤러(FC)에서 수신기(RX)로 텔레메트리 데이터 전송
- 수신기(RX)에서 송신기(TX)로 텔레메트리 데이터 전달
- 양방향 통신 지원

## Setup

PlatformIO 설치

```
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```


