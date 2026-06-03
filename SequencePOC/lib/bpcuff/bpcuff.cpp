#include "bpcuff.h"

CuffControl::CuffControl() {
  ambientPressure = 0.0;
  dataCount = 0;

  holdState = HOLD_IDLE;
  holdStateStart_ms = 0;
  holdStart_ms = 0;

  deflating = false;
}

// ============================================================
// SETUP HARDWARE
// ============================================================

void CuffControl::setupHardware() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(VALVE, OUTPUT);

  stopPump();
  valveClose();

#if defined(ARDUINO_ARCH_RP2040)
  SPI.setSCK(LPS_SCK);
  SPI.setRX(LPS_MISO);
  SPI.setTX(LPS_MOSI);
#endif

  SPI.begin();

  delay(100);

  if (!sensor.begin_SPI(LPS_CS, &SPI)) {
    Serial.println("ERROR: Sensor not found");
    while (1) {
      delay(10);
    }
  }

  delay(500);

  // Deflate once during startup before zeroing
  stopPump();
  valveOpen();
  delay(3000);
  valveClose();
  delay(500);

  ambientPressure = readPressure();

  Serial.println("READY");
  Serial.println("Commands: bp, hold5, deflate, stop");
}

// ============================================================
// PUMP AND VALVE
// ============================================================

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

// ============================================================
// PRESSURE
// ============================================================

float CuffControl::readPressure() {
  return sensor.readPressure();   // hPa
}

float CuffControl::getPressureMmHg() {
  float currentPressure = readPressure();
  float gauge_hPa = currentPressure - ambientPressure;
  float gauge_mmHg = gauge_hPa * 0.750062;

  return gauge_mmHg;
}

// ============================================================
// STOP EVERYTHING
// ============================================================

void CuffControl::stopAll() {
  stopPump();
  valveClose();

  holdState = HOLD_IDLE;
  deflating = false;

  Serial.println("STOPPED");
}

// ============================================================
// START NON-BLOCKING 5-MINUTE HOLD
// ============================================================

void CuffControl::startHold5() {
  stopPump();
  valveOpen();

  holdState = HOLD_ZERO_DEFLATE;
  holdStateStart_ms = millis();

  Serial.println("HOLD5_START");
}

// ============================================================
// START NON-BLOCKING FULL DEFLATE
// ============================================================

void CuffControl::startDeflate() {
  holdState = HOLD_IDLE;

  stopPump();
  valveOpen();

  deflating = true;

  Serial.println("DEFLATE_START");
}

// ============================================================
// 1) BLOCKING FULL BP MEASUREMENT
// ============================================================
//
// This pauses all other code until measurement finishes.
// Only final BP result is printed by calculateBP().
// ============================================================

void CuffControl::bpUpdate() {
  resetBPData();

  // Fully deflate before zeroing
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
      Serial.println("BP_ERROR_PUMP_TIMEOUT");
      return;
    }

    delay(20);
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

    if (now - lastSample >= 10) {
      lastSample = now;

      if (pressure < 155.0 && pressure > 45.0) {
        savePoint(pressure);
      }
    }

    if (now - deflateStart > 60000UL) {
      Serial.println("BP_ERROR_DEFLATE_TIMEOUT");
      break;
    }

    delay(2);
  }

  // Fully deflate after BP measurement
  valveOpen();
  delay(3000);
  valveClose();

  calculateBP();

  Serial.println("BP_DONE");
}

// ============================================================
// 2) NON-BLOCKING 5-MINUTE INFLATION / HOLD / DEFLATE
// ============================================================
//
// Call this repeatedly from loop().
// It does not pause spirometer, HR, or other code.
// ============================================================

void CuffControl::bpInflate5Min() {
  unsigned long now = millis();

  switch (holdState) {
    case HOLD_IDLE:
      return;

    case HOLD_ZERO_DEFLATE:
      if (now - holdStateStart_ms >= 3000UL) {
        valveClose();
        holdState = HOLD_ZERO_SETTLE;
        holdStateStart_ms = now;
      }
      break;

    case HOLD_ZERO_SETTLE:
      if (now - holdStateStart_ms >= 500UL) {
        ambientPressure = readPressure();

        valveClose();
        pumpForward();

        holdState = HOLD_INFLATE;
        holdStateStart_ms = now;
      }
      break;

    case HOLD_INFLATE:
      if (getPressureMmHg() >= 160.0) {
        stopPump();
        valveClose();

        holdStart_ms = now;

        holdState = HOLD_ACTIVE;
        holdStateStart_ms = now;

        Serial.println("HOLD5_PRESSURE_REACHED");
      }

      else if (now - holdStateStart_ms > 30000UL) {
        stopPump();
        valveOpen();

        holdState = HOLD_IDLE;
        deflating = true;

        Serial.println("HOLD5_ERROR_PUMP_TIMEOUT");
      }
      break;

    case HOLD_ACTIVE:
      if (now - holdStart_ms >= 300000UL) {
        stopPump();
        valveOpen();

        holdState = HOLD_DEFLATE;
        holdStateStart_ms = now;

        Serial.println("HOLD5_DONE_DEFLATING");
      }
      break;

    case HOLD_DEFLATE:
      if (getPressureMmHg() <= 5.0) {
        valveClose();

        holdState = HOLD_IDLE;

        Serial.println("HOLD5_FULLY_DEFLATED");
      }

      else if (now - holdStateStart_ms > 30000UL) {
        // Safety: leave valve open if pressure does not drop low enough
        holdState = HOLD_IDLE;

        Serial.println("HOLD5_DEFLATE_TIMEOUT_VALVE_LEFT_OPEN");
      }
      break;
  }
}

// ============================================================
// 3) NON-BLOCKING FULL DEFLATE
// ============================================================
//
// Call this repeatedly from loop().
// It does not pause other code.
// ============================================================

void CuffControl::bpDeflate() {
  if (!deflating) {
    return;
  }

  stopPump();
  valveOpen();

  if (getPressureMmHg() <= 5.0) {
    valveClose();

    deflating = false;

    Serial.println("DEFLATE_DONE");
  }
}

// ============================================================
// DATA HELPERS
// ============================================================

void CuffControl::resetBPData() {
  dataCount = 0;

  for (int i = 0; i < MAX_POINTS; i++) {
    pressureData[i] = 0.0;
    oscData[i] = 0.0;
  }
}

void CuffControl::savePoint(float pressure) {
  if (dataCount >= MAX_POINTS) {
    return;
  }

  pressureData[dataCount] = pressure;
  oscData[dataCount] = 0.0;
  dataCount++;
}