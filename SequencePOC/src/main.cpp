#include <Arduino.h>
#include <bpcuff.h>

CuffControl control;

void setup() {
  Serial.begin(115200);
  delay(1000);

  control.setupHardware();
}

void loop() {
  control.serialControl();
}