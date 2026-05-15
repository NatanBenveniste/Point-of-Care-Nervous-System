#include <Arduino.h>
// -------------------------------
// Motor / valve pins
// -------------------------------
const int IN1   = 3;
const int IN2   = 2;
const int EEP   = 4;
const int VALVE = 10;

// -------------------------------
// Sampling / test settings
// -------------------------------
const unsigned long SAMPLE_INTERVAL_MS = 20;   // 50 Hz
const float TARGET_INFLATE_MMHG = 170.0f;
const float STOP_DEFLATE_MMHG   = 40.0f;

const unsigned long MAX_INFLATE_TIME_MS = 35000;   // 35 sec
const unsigned long HOLD_AT_TARGET_MS   = 4000;    // 4 sec

// Pulsed bleed settings for m()
const unsigned long BLEED_PERIOD_MS = 250;
const unsigned long BLEED_OPEN_MS   = 12;

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(VALVE, OUTPUT);

  stopPump();
  valveClose();

  delay(100);

  Serial.println("READY");
  Serial.setTimeout(10);
}

void loop() {
  serialControl();

  delay(20); // ~50 Hz
}

void serialControl() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  
  if (cmd == "pump") {
    pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    stopPump();
    valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    valveOpen();
    Serial.println("ACK,valve");
  }

  else if (cmd == "close") {
    valveClose();
    Serial.println("ACK,close");
  }

  else if (cmd == "pulse") {
    valvePulse(100);
    Serial.println("ACK,pulse");
  }
}

// -------------------------------
// Pump / valve functions
// -------------------------------
void pumpForward() {
  digitalWrite(EEP, HIGH);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void stopPump() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(EEP, LOW);
}

void valveOpen() {
  digitalWrite(VALVE, HIGH);
}

void valveClose() {
  digitalWrite(VALVE, LOW);
}

void valvePulse(int ms) {
  for (int i = 0; i < 3; i++) {
    valveOpen();
    delay(ms);
    valveClose();
    delay(ms);
  }
}