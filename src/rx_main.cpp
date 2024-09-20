#include <Arduino.h>

HardwareSerial DebugSerial(UART5);

void setup() {
  DebugSerial.setTx(PC_12);
  DebugSerial.setRx(PD_2);
  DebugSerial.begin(115200);

  // Serial.println('setup');
  DebugSerial.println("debug setup");
}
void loop() {
  // Serial.println('loop');
  DebugSerial.println("debug loop");
  delay(1000);
}
