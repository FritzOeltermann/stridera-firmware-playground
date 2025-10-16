#include <Arduino.h>
#include "System.h"

static System sys;

void setup() {
  Serial.begin(115200);
  delay(100);
  sys.begin();         // initializes M5, BLE, IMU, lands in IDLE
}

void loop() {
  sys.loop();          // runs FSM; handles power button, BLE, IMU
}
