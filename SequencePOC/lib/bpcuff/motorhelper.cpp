#include "motorhelper.h"

MotorControl::MotorControl(){
}

//define functions
void MotorControl::pumpForward() {
  digitalWrite(EEP, HIGH);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void MotorControl::stopPump() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(EEP, LOW);
}

void MotorControl::valveOpen() {
  digitalWrite(VALVE, HIGH);
}

void MotorControl::valveClose() {
  digitalWrite(VALVE, LOW);
}

void MotorControl::valvePulse(int ms) {
  for (int i = 0; i < 3; i++) {
    valveOpen();
    delay(ms);
    valveClose();
    delay(ms);
  }
}

void MotorControl::setupHardware() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(VALVE, OUTPUT);

  delay(100);

  Serial.println("Hardware setup");
}

void MotorControl::serialControl() {
  if (!Serial.available()) return;


  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  
  if (cmd == "pump") {
    MotorControl::pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    MotorControl::stopPump();
    MotorControl::valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    MotorControl::valveOpen();
    Serial.println("ACK,valve");
  }

  else if (cmd == "close") {
    MotorControl::valveClose();
    Serial.println("ACK,close");
  }

  else if (cmd == "pulse") {
    MotorControl::valvePulse(100);
    Serial.println("ACK,pulse");
  }
}