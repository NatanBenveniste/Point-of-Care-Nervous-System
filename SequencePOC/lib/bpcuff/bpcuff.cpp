#include "bpcuff.h"

CuffControl::CuffControl(){
}

//define functions
void CuffControl::pumpForward() {
  digitalWrite(EEP, HIGH);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void CuffControl::stopPump() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(EEP, LOW);
}

void CuffControl::valveOpen() {
  digitalWrite(VALVE, HIGH);
}

void CuffControl::valveClose() {
  digitalWrite(VALVE, LOW);
}

void CuffControl::valvePulse(int ms) {
  for (int i = 0; i < 3; i++) {
    valveOpen();
    delay(ms);
    valveClose();
    delay(ms);
  }
}

void CuffControl::setupHardware() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(VALVE, OUTPUT);

  delay(100);

  Serial.println("Hardware setup");
}

void CuffControl::serialControl() {
  if (!Serial.available()) return;


  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  
  if (cmd == "pump") {
    CuffControl::pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    CuffControl::stopPump();
    CuffControl::valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    CuffControl::valveOpen();
    Serial.println("ACK,valve");
  }

  else if (cmd == "close") {
    CuffControl::valveClose();
    Serial.println("ACK,close");
  }

  else if (cmd == "pulse") {
    CuffControl::valvePulse(100);
    Serial.println("ACK,pulse");
  }
}