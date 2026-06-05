#include <Arduino.h>
#include <heartrate.h>
#include "bpcuff.h"
#include "SpirometerVolume.h"

#define BUTTON_PIN 27
#define STOP_BUTTON_PIN 20

// Store measured BP values
float systolic_mmHg = 0.0;
float diastolic_mmHg = 0.0;

// Store baseline HR and HRV for later comparison
float hr_baseline = 0.0f;
float hrv_baseline = 0.0f;

// Store HR and HRV during cuff hold for later comparison
float hr_cuff = 0.0f;
float hrv_cuff = 0.0f;

// Store HR and HRV during spirometer hold for later comparison
float hr_spirometer = 0.0f;
float hrv_spirometer = 0.0f;

// Create library objects
CuffControl bpCuff;
HeartRateMonitor hrm;


// ------------------------------------------------------------
// Main system states
// ------------------------------------------------------------
enum SystemState {
  STATE_SYSTEM_START,
  STATE_GET_BASELINE_HRV,
  STATE_GET_BP,
  STATE_INFLATE,
  STATE_HOLD,
  STATE_DEFLATE,
  STATE_SPIROMETER_HOLD,
  STATE_DONE,
  STATE_ERROR
};

SystemState state = STATE_SYSTEM_START;


// ============================================================
// BUTTON / STATE CONTROL GLOBALS
// ============================================================

bool stateDone = false;

bool lastButtonState = HIGH;
unsigned long lastDebounce_ms = 0;
const unsigned long debounceDelay_ms = 40;


// ============================================================
// PRINT HRV RESULTS
// ============================================================

void printResults(const char* label) {
  Serial.print(label);
  Serial.print(" HR: ");
  Serial.print(hrm.hr);
  Serial.print(" BPM | RMSSD: ");
  Serial.print(hrm.rmssd);
  Serial.println(" ms");
}


// ============================================================
// MAIN HRV UPDATE FUNCTION
// Passed into other phases to update HRV during cuff hold and spirometer hold.
// ============================================================

void updateHRV() {
    if (!hrm.collecting) {
        return;
    }

    hrm.updateRaw();  

    if (hrm.windowElapsed()) {
        hrm.ptProcess();
        hrm.clearVecs();
        hrm.windowCount++;

        if (hrm.windowCount >= hrm.targetWindows) {
            hrm.hrStats(hrm.rrIntervals);
            hrm.collecting = false;
        } 
        else {
            hrm.windowStart = micros();
        }
    }
}


// ============================================================
// BUTTON PRESS HELPER
// INPUT_PULLUP:
// HIGH = not pressed
// LOW  = pressed
// Returns true once per button press.
// ============================================================

bool buttonPressed() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Detect falling edge: HIGH -> LOW
  if (lastButtonState == HIGH && currentButtonState == LOW) {

    // Ignore mechanical bounce
    if (millis() - lastDebounce_ms > debounceDelay_ms) {
      lastDebounce_ms = millis();
      lastButtonState = currentButtonState;
      return true;
    }
  }

  lastButtonState = currentButtonState;
  return false;
}

bool stopButtonPressed() {
  return digitalRead(STOP_BUTTON_PIN) == LOW;
}

void emergencyStopReset() {
  Serial.println("EMERGENCY_STOP");

  // Safety actions first
  bpCuff.stopPump();
  bpCuff.valveOpen();     // deflate cuff

  // Stop/reset active measurements
  spirometerResetTest();

  // If  HRV library has a reset/stop function, call it here

  // Reset state machine flags
  stateDone = false;

  // Return to first state
  state = STATE_SYSTEM_START;

  Serial.println("STATE_SYSTEM_START");
}

// ============================================================
// STATE ADVANCE HELPER
// Called only when button is pressed AND current state is done.
// ============================================================

void goToNextState() {
  stateDone = false;

  switch (state) {

    case STATE_SYSTEM_START:
      state = STATE_GET_BASELINE_HRV;

      hrm.beginMeasurement(60);

      Serial.println("STATE_GET_BASELINE_HRV");
      break;

    case STATE_GET_BASELINE_HRV:
      state = STATE_GET_BP;

      Serial.println("STATE_GET_BP");
      break;

    case STATE_GET_BP:
      state = STATE_INFLATE;

      Serial.println("STATE_INFLATE");
      break;

    case STATE_INFLATE:
      state = STATE_HOLD;

      hrm.beginMeasurement(60);

      Serial.println("STATE_HOLD");
      break;

    case STATE_HOLD:
      state = STATE_DEFLATE;

      Serial.println("STATE_DEFLATE");
      break;

    case STATE_DEFLATE:
      state = STATE_SPIROMETER_HOLD;

      // Start 1-minute spirometer test when entering this state
      spirometerStartTest();
      hrm.beginMeasurement(60);

      Serial.println("STATE_SPIROMETER_HOLD");
      break;

    case STATE_SPIROMETER_HOLD:
      state = STATE_DONE;
      Serial.println("STATE_DONE");
      break;

    default:
      break;
  }
}


// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  // Internal pullup: HIGH = not pressed, LOW = pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  
  hrm.init(); // TODO: bool with error 
  bool cuffOK = bpCuff.bpCuffBegin();

  // Initialize spirometer pressure sensor and zero to atmosphere
  spirometerBegin();

  if (!cuffOK) {
    Serial.println("CUFF_SENSOR_FAILED");
    state = STATE_ERROR;
    return;
  }

  Serial.println("SYSTEM_START");
  Serial.println("Press the button to start collecting ECG data.");
}


// ============================================================
// MAIN LOOP
// Each state runs until complete.
// Button press moves to next state only after stateDone = true.
// ============================================================

void loop() {

  // ============================================================
  // EMERGENCY STOP BUTTON CHECK
  // ============================================================
  if (stopButtonPressed()) {
    emergencyStopReset();
    return;   // prevents the normal state machine from running this loop
  }

  bool pressed = buttonPressed();

  // Advance only when current state has finished
  if (pressed && stateDone) {
    goToNextState();
  }

  switch (state) {

    // ============================================================
    // SYSTEM START
    // ============================================================
    case STATE_SYSTEM_START: {
      
      if (buttonPressed()) {
        goToNextState();
      }
      break;
    }

    // ============================================================
    // 1. HRV BASELINE
    // ============================================================
    case STATE_GET_BASELINE_HRV: {

      updateHRV();

      if (!hrm.collecting && !stateDone) {
        hr_baseline = hrm.hr;
        hrv_baseline = hrm.rmssd;

        Serial.println("BASELINE_DONE");

        Serial.print("Baseline HR: ");
        Serial.print(hr_baseline);
        Serial.print(" BPM | Baseline RMSSD: ");
        Serial.println(hrv_baseline);
        Serial.println("Press the button to start blood pressure measurement.");

        hrm.hr = 0;
        hrm.rmssd = 0;

        stateDone = true;
      }

      break;
    }


    // ============================================================
    // 2. FULL BLOOD PRESSURE TEST
    // ============================================================
    case STATE_GET_BP: {

      if (!stateDone) {
        BPReading bp = bpCuff.GetBloodPressure();

        if (bp.complete) {
          if (bp.valid) {
            systolic_mmHg = bp.systolic;
            diastolic_mmHg = bp.diastolic;

            Serial.print("SYSTOLIC,");
            Serial.println(systolic_mmHg);

            Serial.print("DIASTOLIC,");
            Serial.println(diastolic_mmHg);

            Serial.println("BP_DONE");
            Serial.println("Press the button to inflate the cuff.");

            stateDone = true;
          } 
          else {
            Serial.println("BP_FAILED");
            state = STATE_ERROR;
          }
        }
      }

      break;
    }


    // ============================================================
    // 3. INFLATE TO SYSTOLIC
    // ============================================================
    case STATE_INFLATE: {

      if (!stateDone) {
        bool done = bpCuff.Inflate(systolic_mmHg);

        if (done) {
          Serial.println("INFLATE_DONE");
          Serial.println("Press the button to hold the cuff.");
          stateDone = true;
        }
      }

      break;
    }


    // ============================================================
    // 4. HOLD CUFF / UPDATE HRV
    // ============================================================
    case STATE_HOLD: {

      if (!stateDone) {
        bool done = bpCuff.Hold(updateHRV);

        if (done) {
          hr_cuff = hrm.hr;
          hrv_cuff = hrm.rmssd;

          Serial.println("HOLD_DONE");

          Serial.print("Cuff HR: ");
          Serial.print(hr_cuff);
          Serial.print(" BPM | Cuff RMSSD: ");
          Serial.println(hrv_cuff);
          Serial.println("Press the button to deflate the cuff and start the spirometer test.");

          hrm.hr = 0;
          hrm.rmssd = 0;

          stateDone = true;
        }
      }

      break;
    }


    // ============================================================
    // 5. DEFLATE CUFF
    // ============================================================
    case STATE_DEFLATE: {

      if (!stateDone) {
        bool done = bpCuff.Deflate();

        if (done) {
          Serial.println("DEFLATE_DONE");
          Serial.println("Press the button to start the spirometer test.");
          stateDone = true;
        }
      }

      break;
    }


    // ============================================================
    // 6. SPIROMETER 1-MINUTE TEST
    // ============================================================
    case STATE_SPIROMETER_HOLD: {

      if (!stateDone) {

        // Run spirometer test repeatedly until timer finishes
        bool done = spirometerRunTest();

        // Keep HRV updating during spirometer period
        updateHRV();

        if (done) {
          hr_spirometer = hrm.hr;
          hrv_spirometer = hrm.rmssd;

          Serial.println("SPIROMETER_DONE");

          Serial.print("Spirometer HR: ");
          Serial.print(hr_spirometer); 
          Serial.print(" BPM | Spirometer RMSSD: ");
          Serial.println(hrv_spirometer);
          Serial.println("Press the button to finish.");

          hrm.hr = 0;
          hrm.rmssd = 0;

          stateDone = true;
        }
      }

      break;
    }


    // ============================================================
    // 7. FINAL DONE STATE
    // ============================================================
    case STATE_DONE: {

      // Final idle state
      break;
    }


    // ============================================================
    // ERROR STATE
    // ============================================================
    case STATE_ERROR: {

      // Safety: release cuff pressure
      bpCuff.Deflate();
      break;
    }
  }
}