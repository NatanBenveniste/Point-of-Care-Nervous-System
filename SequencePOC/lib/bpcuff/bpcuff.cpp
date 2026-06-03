#include "bpcuff.h"

// ============================================================
// CONSTRUCTORS
// ============================================================

CuffControl::CuffControl()
  : CuffControl(BP_CUFF_IN1_PIN,
                BP_CUFF_IN2_PIN,
                BP_CUFF_EEP_PIN,
                BP_CUFF_VALVE_PIN,
                BP_CUFF_LPS_CS_PIN) {}

CuffControl::CuffControl(uint8_t in1Pin,
                         uint8_t in2Pin,
                         uint8_t eepPin,
                         uint8_t valvePin,
                         uint8_t lpsCsPin) {
  _in1Pin = in1Pin;
  _in2Pin = in2Pin;
  _eepPin = eepPin;
  _valvePin = valvePin;
  _lpsCsPin = lpsCsPin;

  state = BPCuffState::IDLE;

  stateStartMs = 0;
  lastSampleMs = 0;
  holdStartMs = 0;

  ambientPressure_hPa = 0.0f;

  dataCount = 0;

  result.systolic = -1.0f;
  result.diastolic = -1.0f;
  result.map = -1.0f;
  result.valid = false;
  result.complete = false;

  manualInflateTargetMmHg = HOLD_FALLBACK_TARGET_MMHG;

  holdStarted = false;
  holdStartPressure = 0.0f;
  holdEndPressure = 0.0f;
}

// ============================================================
// BEGIN
// ============================================================

bool CuffControl::bpCuffBegin() {
  pinMode(_in1Pin, OUTPUT);
  pinMode(_in2Pin, OUTPUT);
  pinMode(_eepPin, OUTPUT);
  pinMode(_valvePin, OUTPUT);

  stopPump();
  valveClose();

  bool sensorOK = sensor.begin_SPI(_lpsCsPin, &SPI);

  if (!sensorOK) {
    state = BPCuffState::BP_ERROR;
    return false;
  }

  zeroAmbientNow();

  state = BPCuffState::IDLE;
  return true;
}

// ============================================================
// BASIC HARDWARE CONTROL
// ============================================================

void CuffControl::pumpForward() {
  digitalWrite(_eepPin, HIGH);
  digitalWrite(_in1Pin, HIGH);
  digitalWrite(_in2Pin, LOW);
}

void CuffControl::stopPump() {
  digitalWrite(_in1Pin, LOW);
  digitalWrite(_in2Pin, LOW);
  digitalWrite(_eepPin, LOW);
}

// HIGH = valve open
// LOW  = valve closed

void CuffControl::valveOpen() {
  digitalWrite(_valvePin, HIGH);
}

void CuffControl::valveClose() {
  digitalWrite(_valvePin, LOW);
}

// ============================================================
// PRESSURE
// ============================================================

float CuffControl::readPressure_hPa() {
  return sensor.readPressure();
}

float CuffControl::getPressureMmHg() {
  float currentPressure_hPa = readPressure_hPa();
  float gauge_hPa = currentPressure_hPa - ambientPressure_hPa;
  float gauge_mmHg = gauge_hPa * 0.750062f;

  return gauge_mmHg;
}

void CuffControl::zeroAmbientNow() {
  ambientPressure_hPa = readPressure_hPa();
}

// ============================================================
// INTERNAL STATE HELPERS
// ============================================================

void CuffControl::startState(BPCuffState newState) {
  state = newState;
  stateStartMs = millis();
}

void CuffControl::resetBPData() {
  dataCount = 0;

  for (int i = 0; i < MAX_POINTS; i++) {
    pressureData[i] = 0.0f;
    oscData[i] = 0.0f;
  }
}

void CuffControl::savePoint(float pressureMmHg) {
  if (dataCount >= MAX_POINTS) {
    return;
  }

  pressureData[dataCount] = pressureMmHg;
  oscData[dataCount] = 0.0f;

  dataCount++;
}

void CuffControl::failMeasurement() {
  stopPump();
  valveOpen();

  result.systolic = -1.0f;
  result.diastolic = -1.0f;
  result.map = -1.0f;
  result.valid = false;
  result.complete = true;

  state = BPCuffState::BP_ERROR;
}

// ============================================================
// START FULL BLOOD PRESSURE MEASUREMENT
// This replaces the old Serial command "bp".
// ============================================================

void CuffControl::StartBloodPressure() {
  resetBPData();

  result.systolic = -1.0f;
  result.diastolic = -1.0f;
  result.map = -1.0f;
  result.valid = false;
  result.complete = false;

  stopPump();
  valveOpen();

  startState(BPCuffState::BP_ZERO_DEFLATE);
}

// ============================================================
// FULL NONBLOCKING BLOOD PRESSURE ROUTINE
// Call this repeatedly from loop.
// First call starts the full BP sequence automatically.
// Later calls continue it.
// When done, result.complete becomes true.
// ============================================================

BPReading CuffControl::GetBloodPressure() {
  unsigned long now = millis();

  // First call automatically starts the original BP measurement sequence
  if (state == BPCuffState::IDLE && result.complete == false) {
    StartBloodPressure();
    return result;
  }

  // If already done or failed, just return the result
  if (state == BPCuffState::BP_DONE || state == BPCuffState::BP_ERROR) {
    return result;
  }

  switch (state) {

    // ------------------------------------------------------------
    // 1. Fully dump cuff pressure before BP measurement
    // ------------------------------------------------------------
    case BPCuffState::BP_ZERO_DEFLATE: {
      stopPump();
      valveOpen();

      if (now - stateStartMs >= ZERO_DEFLATE_MS) {
        valveClose();
        startState(BPCuffState::BP_ZERO_SETTLE);
      }

      break;
    }

    // ------------------------------------------------------------
    // 2. Let sensor settle, then zero ambient pressure
    // ------------------------------------------------------------
    case BPCuffState::BP_ZERO_SETTLE: {
      stopPump();
      valveClose();

      if (now - stateStartMs >= ZERO_SETTLE_MS) {
        zeroAmbientNow();

        valveClose();
        pumpForward();

        startState(BPCuffState::BP_INFLATE);
      }

      break;
    }

    // ------------------------------------------------------------
    // 3. Inflate for BP measurement
    // ------------------------------------------------------------
    case BPCuffState::BP_INFLATE: {
      valveClose();
      pumpForward();

      float pressureMmHg = getPressureMmHg();

      if (pressureMmHg >= BP_INFLATE_TARGET_MMHG) {
        stopPump();
        valveClose();

        startState(BPCuffState::BP_SETTLE);
      }
      else if (now - stateStartMs > PUMP_TIMEOUT_MS) {
        failMeasurement();
      }

      break;
    }

    // ------------------------------------------------------------
    // 4. Brief settle after inflation
    // ------------------------------------------------------------
    case BPCuffState::BP_SETTLE: {
      stopPump();
      valveClose();

      if (now - stateStartMs >= BP_SETTLE_MS) {
        valveOpen();
        lastSampleMs = now;

        startState(BPCuffState::BP_DEFLATE_SAMPLE);
      }

      break;
    }

    // ------------------------------------------------------------
    // 5. Deflate while collecting pressure waveform data
    // ------------------------------------------------------------
    case BPCuffState::BP_DEFLATE_SAMPLE: {
      stopPump();
      valveOpen();

      float pressureMmHg = getPressureMmHg();

      if (now - lastSampleMs >= BP_SAMPLE_PERIOD_MS) {
        lastSampleMs = now;

        if (pressureMmHg < BP_SAMPLE_HIGH_MMHG &&
            pressureMmHg > BP_SAMPLE_LOW_MMHG) {
          savePoint(pressureMmHg);
        }
      }

      if (pressureMmHg <= BP_STOP_DEFLATE_MMHG ||
          dataCount >= MAX_POINTS ||
          now - stateStartMs > BP_DEFLATE_TIMEOUT_MS) {

        valveOpen();
        startState(BPCuffState::BP_FINAL_DEFLATE);
      }

      break;
    }

    // ------------------------------------------------------------
    // 6. Fully deflate cuff, then calculate systolic/diastolic
    // ------------------------------------------------------------
    case BPCuffState::BP_FINAL_DEFLATE: {
      stopPump();
      valveOpen();

      float pressureMmHg = getPressureMmHg();

      if (pressureMmHg <= FULL_DEFLATE_DONE_MMHG ||
          now - stateStartMs > FULL_DEFLATE_TIMEOUT_MS) {

        valveClose();

        result.valid = calculateBP(result);
        result.complete = true;

        state = BPCuffState::BP_DONE;
      }

      break;
    }

    default: {
      break;
    }
  }

  return result;
}

// ============================================================
// GRANULAR NONBLOCKING INFLATE
// Used after BP measurement during the hold workflow.
// Pump ON until target pressure.
// Then pump OFF, valve CLOSED.
// ============================================================

bool CuffControl::Inflate() {
  float targetMmHg = HOLD_FALLBACK_TARGET_MMHG;

  if (result.valid && result.systolic > 0.0f) {
    targetMmHg = result.systolic;
  }

  return Inflate(targetMmHg);
}

bool CuffControl::Inflate(float targetMmHg) {
  unsigned long now = millis();

  // First call starts manual inflate
  if (state != BPCuffState::MANUAL_INFLATE) {
    manualInflateTargetMmHg = targetMmHg;

    valveClose();
    pumpForward();

    startState(BPCuffState::MANUAL_INFLATE);
    return false;
  }

  // Later calls continue manual inflate
  valveClose();
  pumpForward();

  float pressureMmHg = getPressureMmHg();

  if (pressureMmHg >= manualInflateTargetMmHg) {
    stopPump();
    valveClose();

    state = BPCuffState::IDLE;
    return true;
  }

  if (now - stateStartMs > PUMP_TIMEOUT_MS) {
    stopPump();
    valveOpen();

    state = BPCuffState::BP_ERROR;
    return true;
  }

  return false;
}

// ============================================================
// GRANULAR NONBLOCKING HOLD
// Pump OFF, valve CLOSED.
// Runs UpdateHRV during the hold.
// Done after 5 minutes.
// ============================================================

bool CuffControl::Hold(void (*updateHRV)()) {
  unsigned long now = millis();

  // First call starts hold
  if (state != BPCuffState::HOLD_ACTIVE) {
    stopPump();
    valveClose();

    holdStarted = true;
    holdStartMs = now;
    holdStartPressure = getPressureMmHg();
    holdEndPressure = holdStartPressure;

    startState(BPCuffState::HOLD_ACTIVE);
    return false;
  }

  // Later calls continue hold
  stopPump();
  valveClose();

  if (updateHRV != nullptr) {
    updateHRV();
  }

  holdEndPressure = getPressureMmHg();

  if (now - holdStartMs >= HOLD_DURATION_MS) {
    state = BPCuffState::IDLE;
    return true;
  }

  return false;
}

// ============================================================
// GRANULAR NONBLOCKING DEFLATE
// Pump OFF, valve OPEN.
// Done when cuff pressure is near zero.
// ============================================================

bool CuffControl::Deflate() {
  unsigned long now = millis();

  // First call starts manual deflate
  if (state != BPCuffState::MANUAL_DEFLATE) {
    stopPump();
    valveOpen();

    startState(BPCuffState::MANUAL_DEFLATE);
    return false;
  }

  // Later calls continue manual deflate
  stopPump();
  valveOpen();

  float pressureMmHg = getPressureMmHg();

  if (pressureMmHg <= FULL_DEFLATE_DONE_MMHG) {
    valveClose();

    state = BPCuffState::IDLE;
    return true;
  }

  if (now - stateStartMs > FULL_DEFLATE_TIMEOUT_MS) {
    // Leave valve open on timeout for safety
    state = BPCuffState::IDLE;
    return true;
  }

  return false;
}

// ============================================================
// STATUS / RESET HELPERS
// ============================================================

bool CuffControl::IsBusy() const {
  return state != BPCuffState::IDLE &&
         state != BPCuffState::BP_DONE &&
         state != BPCuffState::BP_ERROR;
}

BPCuffState CuffControl::getState() const {
  return state;
}

void CuffControl::Reset() {
  stopPump();
  valveClose();

  state = BPCuffState::IDLE;

  holdStarted = false;
  holdStartMs = 0;
  holdStartPressure = 0.0f;
  holdEndPressure = 0.0f;
}

BPReading CuffControl::lastReading() const {
  return result;
}

unsigned long CuffControl::holdElapsedMs() const {
  if (!holdStarted) {
    return 0;
  }

  return millis() - holdStartMs;
}

float CuffControl::holdStartPressureMmHg() const {
  return holdStartPressure;
}

float CuffControl::holdEndPressureMmHg() const {
  return holdEndPressure;
}

float CuffControl::holdPressureDropMmHg() const {
  return holdStartPressure - holdEndPressure;
}

// ============================================================
// BLOOD PRESSURE CALCULATION
// This is the oscillometric processing section from the tested code.
// It uses saved cuff pressure data from deflation.
// ============================================================

bool CuffControl::calculateBP(BPReading &out) {
  if (dataCount < 120) {
    return false;
  }

  float baseline[MAX_POINTS];
  float oscillation[MAX_POINTS];
  float absOsc[MAX_POINTS];
  float envelope[MAX_POINTS];
  float smoothEnvelope[MAX_POINTS];

  // ------------------------------------------------------------
  // 1. Remove slow deflation baseline
  // ------------------------------------------------------------
  const int BASE_WIN = 61;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0f;
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

    if (oscillation[i] < 0.0f) {
      absOsc[i] = -oscillation[i];
    }
    else {
      absOsc[i] = oscillation[i];
    }
  }

  // ------------------------------------------------------------
  // 2. Build envelope from absolute oscillation
  // ------------------------------------------------------------
  const int ENV_WIN = 15;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0f;
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

  // ------------------------------------------------------------
  // 3. Smooth envelope
  // ------------------------------------------------------------
  const int SMOOTH_WIN = 9;

  for (int i = 0; i < dataCount; i++) {
    float sum = 0.0f;
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

  // ------------------------------------------------------------
  // 4. Find MAP
  // ------------------------------------------------------------
  int mapIndex = -1;
  float maxOsc = -1.0f;

  const int edgeSkip = 40;

  if (dataCount < 2 * edgeSkip + 10) {
    return false;
  }

  for (int i = edgeSkip; i < dataCount - edgeSkip; i++) {
    float p = pressureData[i];

    if (p >= 65.0f && p <= 92.0f) {
      if (smoothEnvelope[i] > maxOsc) {
        maxOsc = smoothEnvelope[i];
        mapIndex = i;
      }
    }
  }

  if (mapIndex < 0 || maxOsc <= 0.0f) {
    return false;
  }

  float MAP = pressureData[mapIndex];

  // ------------------------------------------------------------
  // 5. Ratio thresholds
  // ------------------------------------------------------------
  float sbpThreshold = SBP_RATIO * maxOsc;
  float dbpThreshold = DBP_RATIO * maxOsc;

  float SBP = -1.0f;
  float DBP = -1.0f;

  // ------------------------------------------------------------
  // 6. Physiologic ratio-bounded search regions
  // ------------------------------------------------------------
  const float MAP_MARGIN = 3.0f;

  const float SBP_MIN_MAP_RATIO = 1.15f;
  const float SBP_MAX_MAP_RATIO = 1.35f;

  const float DBP_MIN_MAP_RATIO = 0.65f;
  const float DBP_MAX_MAP_RATIO = 0.80f;

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
    return false;
  }

  if (dbpEnd <= dbpStart) {
    return false;
  }

  // ------------------------------------------------------------
  // 7. Find SBP crossing above MAP
  // ------------------------------------------------------------
  for (int i = sbpStart + 1; i <= sbpEnd; i++) {
    float p = pressureData[i];

    if (p <= MAP + MAP_MARGIN) {
      continue;
    }

    if (p < MAP * SBP_MIN_MAP_RATIO ||
        p > MAP * SBP_MAX_MAP_RATIO) {
      continue;
    }

    if (smoothEnvelope[i - 1] < sbpThreshold &&
        smoothEnvelope[i] >= sbpThreshold) {

      float e1 = smoothEnvelope[i - 1];
      float e2 = smoothEnvelope[i];

      float p1 = pressureData[i - 1];
      float p2 = pressureData[i];

      float frac = 0.0f;

      if ((e2 - e1) != 0.0f) {
        frac = (sbpThreshold - e1) / (e2 - e1);
      }

      SBP = p1 + frac * (p2 - p1);
      break;
    }
  }

  // SBP fallback
  if (SBP < 0.0f) {
    float bestDiff = 9999.0f;
    int bestIndex = -1;

    for (int i = sbpStart; i <= sbpEnd; i++) {
      float p = pressureData[i];

      if (p <= MAP + MAP_MARGIN) {
        continue;
      }

      if (p < MAP * SBP_MIN_MAP_RATIO ||
          p > MAP * SBP_MAX_MAP_RATIO) {
        continue;
      }

      float diff = smoothEnvelope[i] - sbpThreshold;

      if (diff < 0.0f) {
        diff = -diff;
      }

      if (diff < bestDiff) {
        bestDiff = diff;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0) {
      SBP = pressureData[bestIndex];
    }
    else {
      return false;
    }
  }

  // ------------------------------------------------------------
  // 8. Find DBP crossing below MAP
  // ------------------------------------------------------------
  for (int i = dbpStart + 1; i <= dbpEnd; i++) {
    float p = pressureData[i];

    if (p >= MAP - MAP_MARGIN) {
      continue;
    }

    if (p < MAP * DBP_MIN_MAP_RATIO ||
        p > MAP * DBP_MAX_MAP_RATIO) {
      continue;
    }

    if (smoothEnvelope[i - 1] > dbpThreshold &&
        smoothEnvelope[i] <= dbpThreshold) {

      float e1 = smoothEnvelope[i - 1];
      float e2 = smoothEnvelope[i];

      float p1 = pressureData[i - 1];
      float p2 = pressureData[i];

      float frac = 0.0f;

      if ((e2 - e1) != 0.0f) {
        frac = (dbpThreshold - e1) / (e2 - e1);
      }

      DBP = p1 + frac * (p2 - p1);
      break;
    }
  }

  // DBP fallback
  if (DBP < 0.0f) {
    float bestDiff = 9999.0f;
    int bestIndex = -1;

    for (int i = dbpStart; i <= dbpEnd; i++) {
      float p = pressureData[i];

      if (p >= MAP - MAP_MARGIN) {
        continue;
      }

      if (p < MAP * DBP_MIN_MAP_RATIO ||
          p > MAP * DBP_MAX_MAP_RATIO) {
        continue;
      }

      float diff = smoothEnvelope[i] - dbpThreshold;

      if (diff < 0.0f) {
        diff = -diff;
      }

      if (diff < bestDiff) {
        bestDiff = diff;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0) {
      DBP = pressureData[bestIndex];
    }
    else {
      return false;
    }
  }

  // ------------------------------------------------------------
  // 9. Validate result
  // ------------------------------------------------------------
  if (SBP < 0.0f || DBP < 0.0f) {
    return false;
  }

  bool validResult = true;

  if (SBP <= MAP) {
    validResult = false;
  }

  if (DBP >= MAP) {
    validResult = false;
  }

  float pulsePressure = SBP - DBP;

  if (pulsePressure < 20.0f || pulsePressure > 80.0f) {
    validResult = false;
  }

  if (SBP < 80.0f || SBP > 170.0f) {
    validResult = false;
  }

  if (DBP < 40.0f || DBP > 110.0f) {
    validResult = false;
  }

  if (!validResult) {
    return false;
  }

  out.systolic = SBP;
  out.diastolic = DBP;
  out.map = MAP;
  out.valid = true;
  out.complete = true;

  return true;
}