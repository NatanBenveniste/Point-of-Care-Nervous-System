#include <Arduino.h>
#include <motorhelper.h>


#define chargePin 26

MotorControl control;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(chargePin, INPUT);

  control.setupHardware();
}

void loop() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  
  if (cmd == "pump") {
    control.pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    control.stopPump();
    control.valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    control.valveOpen();
    Serial.println("ACK,valve");
  }

  else if (cmd == "close") {
    control.valveClose();
    Serial.println("ACK,close");
  }

  else if (cmd == "pulse") {
    control.valvePulse(100);
    Serial.println("ACK,pulse");
  }

  delay(20); // ~50 Hz

long adcraw = analogRead(chargePin);
float batVoltage = (adcraw/1024.0) * 3.3;
Serial.println(adcraw);
Serial.println(batVoltage);
delay(500);
}