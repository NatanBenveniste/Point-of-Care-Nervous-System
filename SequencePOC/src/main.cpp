#include <Arduino.h>
#include "SpirometerVolume.h"

// ============================================================
// TEST SETTINGS
// ============================================================
//
// This main file controls the overall 1-minute test.
//
// The spirometer library handles:
// - pressure reading
// - deltaP filtering
// - breath detection
// - volume integration
// - average breath volume calculation
//
// main.cpp only handles:
// - starting the test timer
// - repeatedly calling spirometerUpdate()
// - printing the final 1-minute result
// ============================================================

const unsigned long TEST_DURATION_MS = 60000;

// ============================================================
// TEST STATE VARIABLES
// ============================================================

bool testRunning = false;
unsigned long testStart_ms = 0;

// ============================================================
// SETUP
// ============================================================
//
// Runs once when the Pico starts.
//
// Serial is started here because main.cpp controls the overall system.
// The spirometer sensor is initialized through spirometerBegin().
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Initialize spirometer pressure sensor and zero to atmosphere
  spirometerBegin();

  // Clear old test data before starting the 1-minute test
  spirometerResetTest();

  // Start the 1-minute test timer
  testRunning = true;
  testStart_ms = millis();

  Serial.println("TEST_START");
}

// ============================================================
// LOOP
// ============================================================
//
// Runs repeatedly.
//
// spirometerUpdate() must be called continuously so the library can:
// - read pressure
// - detect breath starts/stops
// - integrate volume during active breaths
// - store each completed breath volume
//
// The final result is printed only once after 60 seconds.
// ============================================================

void loop() {
  // Keep spirometer measurement running
  spirometerUpdate();

  // ------------------------------------------------------------
  // END 1-MINUTE TEST
  // ------------------------------------------------------------
  //
  // When 60 seconds have passed, stop the test and print:
  //
  // - average completed breath volume
  // - total completed breath count
  //
  // No per-breath results are printed from main.cpp.
  // ------------------------------------------------------------

  if (testRunning && millis() - testStart_ms >= TEST_DURATION_MS) {
    testRunning = false;

    Serial.println("TEST_DONE");

    Serial.print("FINAL_AVERAGE_BREATH_VOLUME_L,");
    Serial.println(spirometerGetAverageBreathVolume_L(), 4);

    Serial.print("TOTAL_BREATHS,");
    Serial.println(spirometerGetBreathCount());
  }
}