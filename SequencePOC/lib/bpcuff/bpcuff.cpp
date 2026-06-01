#include "bpcuff.h"

CuffControl::CuffControl() {
  ambientPressure = 0.0;
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

  if (!sensor.begin_SPI(LPS_CS, &SPI)) {
    while (1) {
      delay(10);
    }
  }

  delay(500);

  // Fully deflate before setting ambient zero
  stopPump();
  valveOpen();
  delay(3000);
  valveClose();
  delay(500);

  ambientPressure = readPressure();
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

// HIGH = valve open
// LOW  = valve closed
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
  if (!Serial.available()) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "pump") {
    valveClose();
    pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    stopPump();
    valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    stopPump();
    valveOpen();
    Serial.println("ACK,valve_open");
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
// reset BP data
// --------------------
void CuffControl::resetBPData() {
  dataCount = 0;

  for (int i = 0; i < MAX_POINTS; i++) {
    pressureData[i] = 0.0;
    oscData[i] = 0.0;
  }
}

// --------------------
// save one raw pressure data point
// --------------------
void CuffControl::savePoint(float pressure) {
  if (dataCount >= MAX_POINTS) {
    return;
  }

  pressureData[dataCount] = pressure;
  oscData[dataCount] = 0.0;
  dataCount++;
}

// --------------------
// BP measurement sequence
// --------------------
void CuffControl::runBPMeasurement() {
  resetBPData();

  // Fully deflate first so ambient zero is correct
  stopPump();
  valveOpen();
  delay(3000);
  valveClose();
  delay(500);

  ambientPressure = readPressure();

  // Inflate cuff
  valveClose();
  pumpForward();

  unsigned long pumpStart = millis();

  while (getPressureMmHg() < 185.0) {
    if (millis() - pumpStart > 30000UL) {
      stopPump();
      valveOpen();
      return;
    }

    delay(100);
  }

  stopPump();
  valveClose();

  delay(1000);

  // Deflate and collect oscillometric data
  valveOpen();

  unsigned long lastSample = millis();
  unsigned long deflateStart = millis();

  while (getPressureMmHg() > 40.0 && dataCount < MAX_POINTS) {
    unsigned long now = millis();
    float pressure = getPressureMmHg();

    // Save at about 100 Hz
    if (now - lastSample >= 10) {
      lastSample = now;

      if (pressure < 155.0 && pressure > 45.0) {
        savePoint(pressure);
      }
    }

    if (now - deflateStart > 60000UL) {
      break;
    }
  }

  // Fully deflate at the end
  valveOpen();
  delay(3000);
  valveClose();

  calculateBP();
}

// --------------------
// pure oscillometric BP calculation
// --------------------
void CuffControl::calculateBP() {
  if (dataCount < 120) {
    return;
  }

  float baseline[MAX_POINTS];
  float oscillation[MAX_POINTS];
  float absOsc[MAX_POINTS];
  float envelope[MAX_POINTS];
  float smoothEnvelope[MAX_POINTS];

  // --------------------------------------------------
  // 1. Remove slow deflation baseline
  // --------------------------------------------------
  const int BASE_WIN = 61;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0;
    int count = 0;

    int start = i - BASE_WIN / 2;
    int end = i + BASE_WIN / 2;

    if (start < 0) {
      start = 0;
    }

    if (end >= dataCount) {
      end = dataCount - 1;
    }

    for (int j = start; j <= end; j++) {
      sum += pressureData[j];
      count++;
    }

    baseline[i] = sum / count;
    oscillation[i] = pressureData[i] - baseline[i];
    oscData[i] = oscillation[i];

    if (oscillation[i] < 0.0) {
      absOsc[i] = -oscillation[i];
    } else {
      absOsc[i] = oscillation[i];
    }
  }

  // --------------------------------------------------
  // 2. Build envelope from absolute oscillation
  // --------------------------------------------------
  const int ENV_WIN = 15;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0;
    int count = 0;

    int start = i - ENV_WIN / 2;
    int end = i + ENV_WIN / 2;

    if (start < 0) {
      start = 0;
    }

    if (end >= dataCount) {
      end = dataCount - 1;
    }

    for (int j = start; j <= end; j++) {
      sum += absOsc[j];
      count++;
    }

    envelope[i] = sum / count;
  }

  // Smooth envelope
  const int SMOOTH_WIN = 9;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0;
    int count = 0;

    int start = i - SMOOTH_WIN / 2;
    int end = i + SMOOTH_WIN / 2;

    if (start < 0) {
      start = 0;
    }

    if (end >= dataCount) {
      end = dataCount - 1;
    }

    for (int j = start; j <= end; j++) {
      sum += envelope[j];
      count++;
    }

    smoothEnvelope[i] = sum / count;
  }

  // --------------------------------------------------
  // 3. Find MAP
  // --------------------------------------------------
  int mapIndex = -1;
  float maxOsc = -1.0;

  const int edgeSkip = 40;

  if (dataCount < 2 * edgeSkip + 10) {
    return;
  }

  for (int i = edgeSkip; i < dataCount - edgeSkip; i++) {
    float p = pressureData[i];

    if (p >= 65.0 && p <= 92.0) {
      if (smoothEnvelope[i] > maxOsc) {
        maxOsc = smoothEnvelope[i];
        mapIndex = i;
      }
    }
  }

  if (mapIndex < 0 || maxOsc <= 0.0) {
    return;
  }

  float MAP = pressureData[mapIndex];

  // --------------------------------------------------
  // 4. Ratio thresholds
  // --------------------------------------------------
  float sbpThreshold = SBP_RATIO * maxOsc;
  float dbpThreshold = DBP_RATIO * maxOsc;

  float SBP = -1.0;
  float DBP = -1.0;

  // --------------------------------------------------
  // 5. Physiologic ratio-bounded search regions
  // --------------------------------------------------
  const float MAP_MARGIN = 3.0;

  const float SBP_MIN_MAP_RATIO = 1.15;
  const float SBP_MAX_MAP_RATIO = 1.35;

  const float DBP_MIN_MAP_RATIO = 0.65;
  const float DBP_MAX_MAP_RATIO = 0.80;

  int sbpStart = edgeSkip;
  int sbpEnd = mapIndex - 1;

  int dbpStart = mapIndex + 1;
  int dbpEnd = dataCount - edgeSkip - 1;

  while (sbpStart < sbpEnd && pressureData[sbpStart] <= MAP + MAP_MARGIN) {
    sbpStart++;
  }

  while (sbpEnd > sbpStart && pressureData[sbpEnd] <= MAP + MAP_MARGIN) {
    sbpEnd--;
  }

  while (dbpStart < dbpEnd && pressureData[dbpStart] >= MAP - MAP_MARGIN) {
    dbpStart++;
  }

  while (dbpEnd > dbpStart && pressureData[dbpEnd] >= MAP - MAP_MARGIN) {
    dbpEnd--;
  }

  if (sbpEnd <= sbpStart) {
    return;
  }

  if (dbpEnd <= dbpStart) {
    return;
  }

  // --------------------------------------------------
  // 6. Find SBP crossing above MAP, inside MAP-ratio bounds
  // --------------------------------------------------
  for (int i = sbpStart + 1; i <= sbpEnd; i++) {
    float p = pressureData[i];

    if (p <= MAP + MAP_MARGIN) {
      continue;
    }

    if (p < MAP * SBP_MIN_MAP_RATIO || p > MAP * SBP_MAX_MAP_RATIO) {
      continue;
    }

    if (smoothEnvelope[i - 1] < sbpThreshold &&
        smoothEnvelope[i] >= sbpThreshold) {

      float e1 = smoothEnvelope[i - 1];
      float e2 = smoothEnvelope[i];
      float p1 = pressureData[i - 1];
      float p2 = pressureData[i];

      float frac = 0.0;

      if ((e2 - e1) != 0.0) {
        frac = (sbpThreshold - e1) / (e2 - e1);
      }

      SBP = p1 + frac * (p2 - p1);
      break;
    }
  }

  // Ratio-bounded SBP fallback
  if (SBP < 0.0) {
    float bestDiff = 9999.0;
    int bestIndex = -1;

    for (int i = sbpStart; i <= sbpEnd; i++) {
      float p = pressureData[i];

      if (p <= MAP + MAP_MARGIN) {
        continue;
      }

      if (p < MAP * SBP_MIN_MAP_RATIO || p > MAP * SBP_MAX_MAP_RATIO) {
        continue;
      }

      float diff = smoothEnvelope[i] - sbpThreshold;

      if (diff < 0.0) {
        diff = -diff;
      }

      if (diff < bestDiff) {
        bestDiff = diff;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0) {
      SBP = pressureData[bestIndex];
    } else {
      return;
    }
  }

  // --------------------------------------------------
  // 7. Find DBP crossing below MAP, inside MAP-ratio bounds
  // --------------------------------------------------
  for (int i = dbpStart + 1; i <= dbpEnd; i++) {
    float p = pressureData[i];

    if (p >= MAP - MAP_MARGIN) {
      continue;
    }

    if (p < MAP * DBP_MIN_MAP_RATIO || p > MAP * DBP_MAX_MAP_RATIO) {
      continue;
    }

    if (smoothEnvelope[i - 1] > dbpThreshold &&
        smoothEnvelope[i] <= dbpThreshold) {

      float e1 = smoothEnvelope[i - 1];
      float e2 = smoothEnvelope[i];
      float p1 = pressureData[i - 1];
      float p2 = pressureData[i];

      float frac = 0.0;

      if ((e2 - e1) != 0.0) {
        frac = (dbpThreshold - e1) / (e2 - e1);
      }

      DBP = p1 + frac * (p2 - p1);
      break;
    }
  }

  // Ratio-bounded DBP fallback
  if (DBP < 0.0) {
    float bestDiff = 9999.0;
    int bestIndex = -1;

    for (int i = dbpStart; i <= dbpEnd; i++) {
      float p = pressureData[i];

      if (p >= MAP - MAP_MARGIN) {
        continue;
      }

      if (p < MAP * DBP_MIN_MAP_RATIO || p > MAP * DBP_MAX_MAP_RATIO) {
        continue;
      }

      float diff = smoothEnvelope[i] - dbpThreshold;

      if (diff < 0.0) {
        diff = -diff;
      }

      if (diff < bestDiff) {
        bestDiff = diff;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0) {
      DBP = pressureData[bestIndex];
    } else {
      return;
    }
  }

  // --------------------------------------------------
  // 8. Validate silently
  // --------------------------------------------------
  if (SBP < 0.0 || DBP < 0.0) {
    return;
  }

  bool validResult = true;

  if (SBP <= MAP) {
    validResult = false;
  }

  if (DBP >= MAP) {
    validResult = false;
  }

  float pulsePressure = SBP - DBP;

  if (pulsePressure < 20.0 || pulsePressure > 80.0) {
    validResult = false;
  }

  if (SBP < 80.0 || SBP > 170.0) {
    validResult = false;
  }

  if (DBP < 40.0 || DBP > 110.0) {
    validResult = false;
  }

  if (!validResult) {
    return;
  }

  // --------------------------------------------------
  // 9. Print only SBP/DBP result
  // --------------------------------------------------
  Serial.print("BP=");
  Serial.print(SBP, 1);
  Serial.print("/");
  Serial.println(DBP, 1);
}

// --------------------
// 5-minute hold sequence
// --------------------
void CuffControl::runFiveMinuteHold() {
  Serial.println("HOLD,start");

  stopPump();
  valveOpen();
  Serial.println("HOLD,zero_deflate_start");
  delay(3000);
  valveClose();
  Serial.println("HOLD,zero_deflate_done");
  delay(500);

  ambientPressure = readPressure();

  Serial.print("ZERO_hPa,");
  Serial.println(ambientPressure, 2);

  valveClose();
  pumpForward();

  unsigned long pumpStart = millis();

  while (getPressureMmHg() < 140.0) {
    float p = getPressureMmHg();

    Serial.print("HOLD_INFLATE,");
    Serial.println(p, 2);

    if (millis() - pumpStart > 30000UL) {
      stopPump();
      valveOpen();
      Serial.println("HOLD,error_pump_timeout");
      return;
    }

    delay(100);
  }

  stopPump();
  valveClose();

  float startPressure = getPressureMmHg();

  Serial.print("HOLD_START_PRESSURE,");
  Serial.println(startPressure, 2);

  unsigned long holdStart = millis();

  while (millis() - holdStart < 300000UL) {
    float p = getPressureMmHg();

    Serial.print("HOLD_DATA,");
    Serial.print((millis() - holdStart) / 1000.0, 1);
    Serial.print(",");
    Serial.println(p, 2);

    delay(1000);
  }

  float endPressure = getPressureMmHg();

  Serial.print("HOLD_END_PRESSURE,");
  Serial.println(endPressure, 2);

  Serial.print("HOLD_DROP,");
  Serial.println(startPressure - endPressure, 2);

  fullDeflate();

  Serial.println("HOLD,done");
}

// --------------------
// full deflate
// --------------------
void CuffControl::fullDeflate() {
  stopPump();
  valveOpen();

  Serial.println("FULL_DEFLATE,start_valve_open");

  unsigned long deflateStart = millis();
  unsigned long lastPrint = millis();

  while (getPressureMmHg() > 5.0) {
    unsigned long now = millis();

    if (now - lastPrint >= 250) {
      lastPrint = now;
      Serial.print("FULL_DEFLATE_PRESSURE,");
      Serial.println(getPressureMmHg(), 2);
    }

    if (now - deflateStart > 30000UL) {
      Serial.println("FULL_DEFLATE,timeout_valve_left_open");
      return;
    }

    delay(20);
  }

  Serial.println("FULL_DEFLATE,done");
  valveClose();
}

/*#include "bpcuff.h"

CuffControl::CuffControl() {
  ambientPressure = 0.0;
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

  if (!sensor.begin_SPI(LPS_CS, &SPI)) {
    Serial.println("ERROR: Sensor not found");
    while (1) {
      delay(10);
    }
  }

  delay(500);

  stopPump();
  valveOpen();
  Serial.println("STARTUP_DEFLATE,valve_open_for_3_seconds");
  delay(3000);
  valveClose();
  Serial.println("STARTUP_DEFLATE,valve_closed");
  delay(500);

  ambientPressure = readPressure();

  Serial.print("ZERO_hPa,");
  Serial.println(ambientPressure, 2);

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

// HIGH = valve open
// LOW  = valve closed
void CuffControl::valveOpen() {
  digitalWrite(VALVE, HIGH);
  Serial.println("DEBUG_VALVE_PIN,HIGH_OPEN");
}

void CuffControl::valveClose() {
  digitalWrite(VALVE, LOW);
  Serial.println("DEBUG_VALVE_PIN,LOW_CLOSED");
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
  if (!Serial.available()) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "pump") {
    valveClose();
    pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    stopPump();
    valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    stopPump();

    Serial.println("ACK,valve_open_10_seconds");

    valveOpen();

    unsigned long startTime = millis();
    unsigned long lastPrint = millis();

    while (millis() - startTime < 10000UL) {
      unsigned long now = millis();

      if (now - lastPrint >= 500) {
        lastPrint = now;
        Serial.print("VALVE_TEST_PRESSURE,");
        Serial.println(getPressureMmHg(), 2);
      }

      delay(20);
    }

    valveClose();

    Serial.println("ACK,valve_closed_after_10_seconds");
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
    Serial.println("ACK,bp");
    runBPMeasurement();
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
// reset BP data
// --------------------
void CuffControl::resetBPData() {
  dataCount = 0;

  for (int i = 0; i < MAX_POINTS; i++) {
    pressureData[i] = 0.0;
    oscData[i] = 0.0;
  }
}

// --------------------
// save one raw pressure data point
// --------------------
void CuffControl::savePoint(float pressure) {
  if (dataCount >= MAX_POINTS) {
    return;
  }

  pressureData[dataCount] = pressure;
  oscData[dataCount] = 0.0;
  dataCount++;
}

// --------------------
// BP measurement sequence
// --------------------
void CuffControl::runBPMeasurement() {
  resetBPData();

  Serial.println("BP,start");

  // Fully deflate first so ambient zero is correct
  stopPump();
  valveOpen();
  Serial.println("BP,zero_deflate_start");
  delay(3000);
  valveClose();
  Serial.println("BP,zero_deflate_done");
  delay(500);

  ambientPressure = readPressure();

  Serial.print("ZERO_hPa,");
  Serial.println(ambientPressure, 2);

  // Inflate cuff
  valveClose();
  pumpForward();

  unsigned long pumpStart = millis();

  while (getPressureMmHg() < 185.0) {
    float p = getPressureMmHg();

    Serial.print("INFLATE,");
    Serial.println(p, 2);

    if (millis() - pumpStart > 30000UL) {
      stopPump();
      valveOpen();
      Serial.println("RESULT,error_pump_timeout");
      return;
    }

    delay(100);
  }

  stopPump();
  valveClose();

  Serial.println("BP,inflation_done");
  delay(1000);

  // Deflate and collect oscillometric data
  valveOpen();
  Serial.println("BP,deflation_start_valve_open");

  unsigned long lastSample = millis();
  unsigned long lastPrint = millis();
  unsigned long deflateStart = millis();

  while (getPressureMmHg() > 40.0 && dataCount < MAX_POINTS) {
    unsigned long now = millis();
    float pressure = getPressureMmHg();

    // Print pressure only every 100 ms.
    // Avoid printing every sample because it disrupts timing.
    if (now - lastPrint >= 100) {
      lastPrint = now;
      Serial.print("DEFLATE,");
      Serial.println(pressure, 2);
    }

    // Save at about 100 Hz
    if (now - lastSample >= 10) {
      lastSample = now;

      if (pressure < 155.0 && pressure > 45.0) {
        savePoint(pressure);
      }
    }

    if (now - deflateStart > 60000UL) {
      Serial.println("RESULT,error_deflate_timeout");
      break;
    }
  }

  // Fully deflate at the end
  valveOpen();
  Serial.println("BP,end_full_deflate");
  delay(3000);
  valveClose();

  calculateBP();

  Serial.println("BP,done");
}

// --------------------
// pure oscillometric BP calculation
// --------------------
void CuffControl::calculateBP() {
  if (dataCount < 120) {
    Serial.print("RESULT,error_not_enough_data,dataCount=");
    Serial.println(dataCount);
    return;
  }

  float baseline[MAX_POINTS];
  float oscillation[MAX_POINTS];
  float absOsc[MAX_POINTS];
  float envelope[MAX_POINTS];
  float smoothEnvelope[MAX_POINTS];

  // --------------------------------------------------
  // 1. Remove slow deflation baseline
  // --------------------------------------------------
  const int BASE_WIN = 61;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0;
    int count = 0;

    int start = i - BASE_WIN / 2;
    int end = i + BASE_WIN / 2;

    if (start < 0) {
      start = 0;
    }

    if (end >= dataCount) {
      end = dataCount - 1;
    }

    for (int j = start; j <= end; j++) {
      sum += pressureData[j];
      count++;
    }

    baseline[i] = sum / count;
    oscillation[i] = pressureData[i] - baseline[i];
    oscData[i] = oscillation[i];

    if (oscillation[i] < 0.0) {
      absOsc[i] = -oscillation[i];
    } else {
      absOsc[i] = oscillation[i];
    }
  }

  // --------------------------------------------------
  // 2. Build envelope from absolute oscillation
  // --------------------------------------------------
  const int ENV_WIN = 15;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0;
    int count = 0;

    int start = i - ENV_WIN / 2;
    int end = i + ENV_WIN / 2;

    if (start < 0) {
      start = 0;
    }

    if (end >= dataCount) {
      end = dataCount - 1;
    }

    for (int j = start; j <= end; j++) {
      sum += absOsc[j];
      count++;
    }

    envelope[i] = sum / count;
  }

  // Smooth envelope
  const int SMOOTH_WIN = 9;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0;
    int count = 0;

    int start = i - SMOOTH_WIN / 2;
    int end = i + SMOOTH_WIN / 2;

    if (start < 0) {
      start = 0;
    }

    if (end >= dataCount) {
      end = dataCount - 1;
    }

    for (int j = start; j <= end; j++) {
      sum += envelope[j];
      count++;
    }

    smoothEnvelope[i] = sum / count;
  }

  // --------------------------------------------------
  // 3. Find MAP
  // --------------------------------------------------
  int mapIndex = -1;
  float maxOsc = -1.0;

  const int edgeSkip = 40;

  if (dataCount < 2 * edgeSkip + 10) {
    Serial.println("RESULT,error_not_enough_data_after_edge_skip");
    return;
  }

  for (int i = edgeSkip; i < dataCount - edgeSkip; i++) {
    float p = pressureData[i];

    // Current best middle-ground range.
    // 65-95 can choose MAP too high.
    // 65-88 made SBP collapse.
    // 65-92 is current range.
    if (p >= 65.0 && p <= 92.0) {
      if (smoothEnvelope[i] > maxOsc) {
        maxOsc = smoothEnvelope[i];
        mapIndex = i;
      }
    }
  }

  if (mapIndex < 0 || maxOsc <= 0.0) {
    Serial.println("RESULT,error_no_valid_map");
    return;
  }

  float MAP = pressureData[mapIndex];

  // --------------------------------------------------
  // 4. Ratio thresholds
  // --------------------------------------------------
  float sbpThreshold = SBP_RATIO * maxOsc;
  float dbpThreshold = DBP_RATIO * maxOsc;

  float SBP = -1.0;
  float DBP = -1.0;

  // --------------------------------------------------
  // 5. Physiologic ratio-bounded search regions
  // --------------------------------------------------
  // Data order during deflation:
  // lower index = higher pressure
  // higher index = lower pressure

  const float MAP_MARGIN = 3.0;

  // Final BP must obey:
  // SBP > MAP > DBP.
  // These are ratio bounds around MAP, not fixed offsets.
  const float SBP_MIN_MAP_RATIO = 1.15;
  const float SBP_MAX_MAP_RATIO = 1.35;

  const float DBP_MIN_MAP_RATIO = 0.65;
  const float DBP_MAX_MAP_RATIO = 0.80;

  int sbpStart = edgeSkip;
  int sbpEnd = mapIndex - 1;

  int dbpStart = mapIndex + 1;
  int dbpEnd = dataCount - edgeSkip - 1;

  while (sbpStart < sbpEnd && pressureData[sbpStart] <= MAP + MAP_MARGIN) {
    sbpStart++;
  }

  while (sbpEnd > sbpStart && pressureData[sbpEnd] <= MAP + MAP_MARGIN) {
    sbpEnd--;
  }

  while (dbpStart < dbpEnd && pressureData[dbpStart] >= MAP - MAP_MARGIN) {
    dbpStart++;
  }

  while (dbpEnd > dbpStart && pressureData[dbpEnd] >= MAP - MAP_MARGIN) {
    dbpEnd--;
  }

  if (sbpEnd <= sbpStart) {
    Serial.println("RESULT,error_no_valid_sbp_search_region");
    return;
  }

  if (dbpEnd <= dbpStart) {
    Serial.println("RESULT,error_no_valid_dbp_search_region");
    return;
  }

  // --------------------------------------------------
  // 6. Find SBP crossing above MAP, inside MAP-ratio bounds
  // --------------------------------------------------
  for (int i = sbpStart + 1; i <= sbpEnd; i++) {
    float p = pressureData[i];

    if (p <= MAP + MAP_MARGIN) {
      continue;
    }

    if (p < MAP * SBP_MIN_MAP_RATIO || p > MAP * SBP_MAX_MAP_RATIO) {
      continue;
    }

    if (smoothEnvelope[i - 1] < sbpThreshold &&
        smoothEnvelope[i] >= sbpThreshold) {

      float e1 = smoothEnvelope[i - 1];
      float e2 = smoothEnvelope[i];
      float p1 = pressureData[i - 1];
      float p2 = pressureData[i];

      float frac = 0.0;

      if ((e2 - e1) != 0.0) {
        frac = (sbpThreshold - e1) / (e2 - e1);
      }

      SBP = p1 + frac * (p2 - p1);
      break;
    }
  }

  // Ratio-bounded SBP fallback
  if (SBP < 0.0) {
    float bestDiff = 9999.0;
    int bestIndex = -1;

    for (int i = sbpStart; i <= sbpEnd; i++) {
      float p = pressureData[i];

      if (p <= MAP + MAP_MARGIN) {
        continue;
      }

      if (p < MAP * SBP_MIN_MAP_RATIO || p > MAP * SBP_MAX_MAP_RATIO) {
        continue;
      }

      float diff = smoothEnvelope[i] - sbpThreshold;
      if (diff < 0.0) {
        diff = -diff;
      }

      if (diff < bestDiff) {
        bestDiff = diff;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0) {
      SBP = pressureData[bestIndex];
      Serial.println("WARN,SBP_exact_crossing_not_found_used_ratio_bounded_above_MAP_candidate");
    } else {
      Serial.println("RESULT,error_sbp_not_found_in_ratio_bounded_region");
      return;
    }
  }

  // --------------------------------------------------
  // 7. Find DBP crossing below MAP, inside MAP-ratio bounds
  // --------------------------------------------------
  for (int i = dbpStart + 1; i <= dbpEnd; i++) {
    float p = pressureData[i];

    if (p >= MAP - MAP_MARGIN) {
      continue;
    }

    if (p < MAP * DBP_MIN_MAP_RATIO || p > MAP * DBP_MAX_MAP_RATIO) {
      continue;
    }

    if (smoothEnvelope[i - 1] > dbpThreshold &&
        smoothEnvelope[i] <= dbpThreshold) {

      float e1 = smoothEnvelope[i - 1];
      float e2 = smoothEnvelope[i];
      float p1 = pressureData[i - 1];
      float p2 = pressureData[i];

      float frac = 0.0;

      if ((e2 - e1) != 0.0) {
        frac = (dbpThreshold - e1) / (e2 - e1);
      }

      DBP = p1 + frac * (p2 - p1);
      break;
    }
  }

  // Ratio-bounded DBP fallback
  if (DBP < 0.0) {
    float bestDiff = 9999.0;
    int bestIndex = -1;

    for (int i = dbpStart; i <= dbpEnd; i++) {
      float p = pressureData[i];

      if (p >= MAP - MAP_MARGIN) {
        continue;
      }

      if (p < MAP * DBP_MIN_MAP_RATIO || p > MAP * DBP_MAX_MAP_RATIO) {
        continue;
      }

      float diff = smoothEnvelope[i] - dbpThreshold;
      if (diff < 0.0) {
        diff = -diff;
      }

      if (diff < bestDiff) {
        bestDiff = diff;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0) {
      DBP = pressureData[bestIndex];
      Serial.println("WARN,DBP_exact_crossing_not_found_used_ratio_bounded_below_MAP_candidate");
    } else {
      Serial.println("RESULT,error_dbp_not_found_in_ratio_bounded_region");
      return;
    }
  }

  // --------------------------------------------------
  // 8. Validate
  // --------------------------------------------------
  if (SBP < 0.0 || DBP < 0.0) {
    Serial.println("RESULT,error_sbp_or_dbp_not_found");
    return;
  }

  bool validResult = true;

  if (SBP <= MAP) {
    Serial.println("WARN,SBP_less_or_equal_MAP");
    validResult = false;
  }

  if (DBP >= MAP) {
    Serial.println("WARN,DBP_greater_or_equal_MAP");
    validResult = false;
  }

  float pulsePressure = SBP - DBP;

  if (pulsePressure < 20.0 || pulsePressure > 80.0) {
    Serial.println("WARN,pulse_pressure_outside_expected_range");
    validResult = false;
  }

  if (SBP < 80.0 || SBP > 170.0) {
    Serial.println("WARN,SBP_outside_expected_range");
    validResult = false;
  }

  if (DBP < 40.0 || DBP > 110.0) {
    Serial.println("WARN,DBP_outside_expected_range");
    validResult = false;
  }

  // --------------------------------------------------
  // 9. Print result
  // --------------------------------------------------
  if (validResult) {
    Serial.print("RESULT,");
  } else {
    Serial.print("RESULT_RATIO_INVALID,");
  }

  Serial.print(SBP, 1);
  Serial.print(",");
  Serial.print(DBP, 1);
  Serial.print(",");
  Serial.println(MAP, 1);

  Serial.print("DEBUG,maxOsc=");
  Serial.print(maxOsc, 4);
  Serial.print(",sbpThreshold=");
  Serial.print(sbpThreshold, 4);
  Serial.print(",dbpThreshold=");
  Serial.print(dbpThreshold, 4);
  Serial.print(",mapIndex=");
  Serial.print(mapIndex);
  Serial.print(",dataCount=");
  Serial.print(dataCount);
  Serial.print(",sbpRegion=");
  Serial.print(sbpStart);
  Serial.print("-");
  Serial.print(sbpEnd);
  Serial.print(",dbpRegion=");
  Serial.print(dbpStart);
  Serial.print("-");
  Serial.print(dbpEnd);
  Serial.print(",MAP=");
  Serial.print(MAP, 1);
  Serial.print(",SBP_MAP_ratio=");
  Serial.print(SBP / MAP, 3);
  Serial.print(",DBP_MAP_ratio=");
  Serial.print(DBP / MAP, 3);
  Serial.print(",validResult=");

  if (validResult) {
    Serial.println("true");
  } else {
    Serial.println("false");
  }

  // Keep graphing output off for stable timing.
  // Re-enable only when debugging envelope shape.
  /*
  Serial.println("OSC_DEBUG_START");
  Serial.println("index,pressure,osc,envelope");

  for (int i = 0; i < dataCount; i++) {
    Serial.print(i);
    Serial.print(",");
    Serial.print(pressureData[i], 2);
    Serial.print(",");
    Serial.print(oscillation[i], 4);
    Serial.print(",");
    Serial.println(smoothEnvelope[i], 4);
  }

  Serial.println("OSC_DEBUG_END");
  
}

// --------------------
// 5-minute hold sequence
// --------------------
void CuffControl::runFiveMinuteHold() {
  Serial.println("HOLD,start");

  stopPump();
  valveOpen();
  Serial.println("HOLD,zero_deflate_start");
  delay(3000);
  valveClose();
  Serial.println("HOLD,zero_deflate_done");
  delay(500);

  ambientPressure = readPressure();

  Serial.print("ZERO_hPa,");
  Serial.println(ambientPressure, 2);

  valveClose();
  pumpForward();

  unsigned long pumpStart = millis();

  while (getPressureMmHg() < 140.0) {
    float p = getPressureMmHg();

    Serial.print("HOLD_INFLATE,");
    Serial.println(p, 2);

    if (millis() - pumpStart > 30000UL) {
      stopPump();
      valveOpen();
      Serial.println("HOLD,error_pump_timeout");
      return;
    }

    delay(100);
  }

  stopPump();
  valveClose();

  float startPressure = getPressureMmHg();

  Serial.print("HOLD_START_PRESSURE,");
  Serial.println(startPressure, 2);

  unsigned long holdStart = millis();

  while (millis() - holdStart < 300000UL) {
    float p = getPressureMmHg();

    Serial.print("HOLD_DATA,");
    Serial.print((millis() - holdStart) / 1000.0, 1);
    Serial.print(",");
    Serial.println(p, 2);

    delay(1000);
  }

  float endPressure = getPressureMmHg();

  Serial.print("HOLD_END_PRESSURE,");
  Serial.println(endPressure, 2);

  Serial.print("HOLD_DROP,");
  Serial.println(startPressure - endPressure, 2);

  fullDeflate();

  Serial.println("HOLD,done");
}

// --------------------
// full deflate
// --------------------
void CuffControl::fullDeflate() {
  stopPump();
  valveOpen();

  Serial.println("FULL_DEFLATE,start_valve_open");

  unsigned long deflateStart = millis();
  unsigned long lastPrint = millis();

  while (getPressureMmHg() > 5.0) {
    unsigned long now = millis();

    if (now - lastPrint >= 250) {
      lastPrint = now;
      Serial.print("FULL_DEFLATE_PRESSURE,");
      Serial.println(getPressureMmHg(), 2);
    }

    if (now - deflateStart > 30000UL) {
      Serial.println("FULL_DEFLATE,timeout_valve_left_open");
      return;
    }

    delay(20);
  }

  Serial.println("FULL_DEFLATE,done");
  valveClose();
}
  */