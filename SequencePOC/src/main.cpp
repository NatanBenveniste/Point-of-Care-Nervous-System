#include <Arduino.h>
#include <bpcuff.h>

CuffControl control;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("serial");

  control.setupHardware();
}

void loop() {
  control.serialControl();
}