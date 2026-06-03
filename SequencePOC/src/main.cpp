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
  
  // Non-blocking functions keep updating here
  control.bpInflate5Min();
  control.bpDeflate();
}