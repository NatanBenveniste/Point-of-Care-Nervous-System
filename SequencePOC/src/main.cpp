#include <Arduino.h>
#include <heartrate.h>

HeartRateMonitor ecg;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("serial");

  ecg.init();
}

void loop() {
  ecg.readECG();
  delay(4);
}