#include <Arduino.h>
#include "bpcuff.h"

// Create cuff object from the library
CuffControl bpCuff;

// Store measured BP values
float systolic_mmHg = 0.0;
float diastolic_mmHg = 0.0;

// ------------------------------------------------------------
// Placeholder HRV function
// You will implement this later
// ------------------------------------------------------------
void UpdateHRV() {
  // HRV update code goes here later
}

// ------------------------------------------------------------
// Main system states
// ------------------------------------------------------------
enum SystemState {
  STATE_GET_BP,
  STATE_INFLATE,
  STATE_HOLD,
  STATE_DEFLATE,
  STATE_DONE,
  STATE_ERROR
};

SystemState state = STATE_GET_BP;

void setup() {
  Serial.begin(115200);

  bool cuffOK = bpCuff.bpCuffBegin();

  if (!cuffOK) {
    Serial.println("CUFF_SENSOR_FAILED");
    state = STATE_ERROR;
    return;
  }

  Serial.println("SYSTEM_START");
}

void loop() {

  switch (state) {

    // ============================================================
    // 1. FULL BLOOD PRESSURE MEASUREMENT
    // This replaces the old Serial command "bp".
    // The function starts automatically and runs by itself.
    // ============================================================
    case STATE_GET_BP: {
      BPReading bp = bpCuff.GetBloodPressure();

      if (bp.complete) {
        if (bp.valid) {
          systolic_mmHg = bp.systolic;
          diastolic_mmHg = bp.diastolic;

          state = STATE_INFLATE;

          Serial.print("SYSTOLIC,");
          Serial.println(systolic_mmHg);

          Serial.print("DIASTOLIC,");
          Serial.println(diastolic_mmHg);
          
        }
        else {
          Serial.println("BP_FAILED");
          state = STATE_ERROR;
        }
      }

      break;
    }

    // ============================================================
    // 2. GRANULAR INFLATE COMMAND
    // Pump ON until cuff reaches measured systolic.
    // Then pump OFF, valve CLOSED.
    // ============================================================
    case STATE_INFLATE: {
      bool done = bpCuff.Inflate(systolic_mmHg);

      if (done) {
        Serial.println("INFLATE_DONE");
        state = STATE_HOLD;
      }

      break;
    }

    // ============================================================
    // 3. GRANULAR HOLD COMMAND
    // Pump OFF, valve CLOSED.
    // Runs UpdateHRV while holding for 5 minutes.
    // ============================================================
    case STATE_HOLD: {
      bool done = bpCuff.Hold(UpdateHRV);

      if (done) {
        Serial.println("HOLD_DONE");
        state = STATE_DEFLATE;
      }

      break;
    }

    // ============================================================
    // 4. GRANULAR DEFLATE COMMAND
    // Pump OFF, valve OPEN.
    // ============================================================
    case STATE_DEFLATE: {
      bool done = bpCuff.Deflate();

      if (done) {
        Serial.println("DEFLATE_DONE");
        state = STATE_DONE;
      }

      break;
    }

    // ============================================================
    // 5. DONE
    // ============================================================
    case STATE_DONE: {
      break;
    }

    // ============================================================
    // ERROR
    // ============================================================
    case STATE_ERROR: {
      bpCuff.Deflate();   // safest behavior: release cuff pressure
      break;
    }
  }
}