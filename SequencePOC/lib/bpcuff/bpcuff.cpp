#include "bpcuff.h"

CuffControl::CuffControl() {
  ambientPressure = 0;
  dataCount = 0;
}

// --------------------
// setup hardware
// --------------------
void CuffControl::setupHardware() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(VALVE, OUTPUT);

  stopPump();
  valveClose();

  delay(100);

  Serial.println("Hardware setup");

  sensor.begin_SPI(LPS_CS, &SPI);
  
  // if (!sensor.begin_SPI(LPS_CS, LPS_SCK, LPS_MISO, LPS_MOSI)) {
  //   Serial.println("ERROR: Sensor not found");
  //   while (1) {
  //     delay(10);
  //   }
  // }

  delay(500);

  ambientPressure = readPressure();

  Serial.println("Sensor setup done");
  Serial.println("Commands: pump, stop, valve, close, pressure, bp, hold5, deflate");
}

// --------------------
// pump and valve
// --------------------
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

// --------------------
// pressure
// --------------------
float CuffControl::readPressure() {
  return sensor.readPressure();   // hPa
}

float CuffControl::getPressureMmHg() {
  float currentPressure = readPressure();
  float gauge_hPa = currentPressure - ambientPressure;
  float gauge_mmHg = gauge_hPa * 0.750062;

  return gauge_mmHg;
}

// --------------------
// serial commands
// --------------------
void CuffControl::serialControl() {
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

  else if (cmd == "pressure") {
    Serial.print("Pressure_mmHg: ");
    Serial.println(getPressureMmHg(), 2);
  }

  else if (cmd == "bp") {
    runBPMeasurement();
    Serial.println("ACK,bp");
    
  }

  else if (cmd == "hold5") {
    Serial.println("ACK,hold5");
    runFiveMinuteHold();
  }

  else if (cmd == "deflate") {
    Serial.println("ACK,deflate");
    fullDeflate();
  }

  else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}

// --------------------
// reset BP arrays
// --------------------
void CuffControl::resetBPData() {
  dataCount = 0;

  for (int i = 0; i < MAX_POINTS; i++) {
    pressureData[i] = 0;
    oscData[i] = 0;
  }
}

// --------------------
// save one oscillation point
// --------------------
void CuffControl::savePoint(float pressure, float osc) {
  if (dataCount >= MAX_POINTS) return;

  pressureData[dataCount] = pressure;
  oscData[dataCount] = osc;
  dataCount++;
}

// --------------------
// BP measurement sequence
// --------------------
void CuffControl::runBPMeasurement() {
  resetBPData();

  // zero pressure before measurement
  stopPump();
  valveOpen();
  delay(2000);
  valveClose();
  delay(500);

  ambientPressure = readPressure();

  // inflate cuff to target pressure
  valveClose();
  pumpForward();

  unsigned long pumpStart = millis();

  while (getPressureMmHg() < 190.0) {
    if (millis() - pumpStart > 30000) {
      stopPump();
      valveOpen();
      Serial.println("RESULT,error_pump_timeout");
      return;
    }

    delay(100);
  }

  stopPump();
  delay(1000);

  // deflate and collect oscillation data
  valveOpen();

  float baseline = getPressureMmHg();

  while (getPressureMmHg() > 40.0) {
    float pressure = getPressureMmHg();

    // slow baseline tracking
    baseline = 0.98 * baseline + 0.02 * pressure;

    // oscillation signal
    float osc = abs(pressure - baseline);

    // save useful points only
    if (osc > 0.05) {
      savePoint(pressure, osc);
    }

    delay(50);
  }

  fullDeflate();

  calculateBP();
}

// --------------------
// calculate SBP / DBP / MAP using calibrated ratio method
// --------------------
void CuffControl::calculateBP() {
  if (dataCount < 10) {
    Serial.println("RESULT,error_not_enough_data");
    return;
  }

  // find max oscillation = MAP point
  int mapIndex = 0;
  float maxOsc = oscData[0];

  for (int i = 1; i < dataCount; i++) {
    if (oscData[i] > maxOsc) {
      maxOsc = oscData[i];
      mapIndex = i;
    }
  }

  float MAP = pressureData[mapIndex];

  float sbpThreshold = SBP_RATIO * maxOsc;
  float dbpThreshold = DBP_RATIO * maxOsc;

  float SBP = 0;
  float DBP = 0;

  // SBP: before MAP, first point that rises above SBP threshold
  for (int i = 0; i <= mapIndex; i++) {
    if (oscData[i] >= sbpThreshold) {
      SBP = pressureData[i];
      break;
    }
  }

  // DBP: after MAP, first point that falls below DBP threshold
  for (int i = mapIndex; i < dataCount; i++) {
    if (oscData[i] <= dbpThreshold) {
      DBP = pressureData[i];
      break;
    }
  }

  if (SBP == 0) {
    SBP = pressureData[0];
  }

  if (DBP == 0) {
    DBP = pressureData[dataCount - 1];
  }

  // final compact result only:
  // RESULT,SBP,DBP,MAP
  Serial.print("RESULT,");
  Serial.print(SBP, 1);
  Serial.print(",");
  Serial.print(DBP, 1);
  Serial.print(",");
  Serial.println(MAP, 1);
}

// --------------------
// 5-minute hold sequence
// --------------------
void CuffControl::runFiveMinuteHold() {
  // zero
  stopPump();
  valveOpen();
  delay(2000);
  valveClose();
  delay(500);

  ambientPressure = readPressure();

  // inflate
  valveClose();
  pumpForward();

  unsigned long pumpStart = millis();

  while (getPressureMmHg() < 180.0) {
    if (millis() - pumpStart > 30000) {
      stopPump();
      valveOpen();
      Serial.println("HOLD,error_pump_timeout");
      return;
    }

    delay(100);
  }

  stopPump();
  valveClose();

  // hold for 5 minutes
  delay(300000UL);

  fullDeflate();

  Serial.println("HOLD,done");
}

// --------------------
// full deflate
// --------------------
void CuffControl::fullDeflate() {
  stopPump();
  valveOpen();

  delay(30000);

  valveClose();
}