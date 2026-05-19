#include <Arduino.h>
#include <motorhelper.h>
#include <displaymanager.h>


#define chargePin 26
bool showCharge = false;

#define buttonPin 27

MotorControl control;
DisplayManager screen;

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(chargePin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);


  control.setupHardware();
  // screen.init();
}

void loop() {
  // screen.runDemo();
  int button = digitalRead(buttonPin);
  if (button == LOW) {
    Serial.println("next");
  }


  if (showCharge) {
    long adcraw = analogRead(chargePin);
    float batVoltage = (adcraw/1024.0) * 3.3;
    Serial.println(adcraw);
    Serial.println(batVoltage);
  }

  delay(50);
}