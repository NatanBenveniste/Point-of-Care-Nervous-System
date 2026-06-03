#include "SpirometerVolume.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

// ============================================================
// SENSOR OBJECT
// ============================================================
//
// This library version uses one pressure sensor.
//
// The pressure sensor measures absolute pressure after laminar flow restriction.
// The code zeros the sensor to atmospheric pressure at startup.
// Then:
//
// deltaP = measured pressure - atmospheric zero pressure
//
// This assumes downstream pressure is atmospheric.
// ============================================================

Adafruit_LPS35HW upstreamSensor = Adafruit_LPS35HW();

// ============================================================
// PIN SETUP
// ============================================================
//
// This is the chip select pin for the upstream pressure sensor.
// ============================================================

const int UPSTREAM_CS_PIN = 17;

// ============================================================
// SETTINGS
// ============================================================

// ------------------------------------------------------------
// Zeroing
// ------------------------------------------------------------
//
// ZERO_SAMPLES controls how many pressure samples are averaged
// to estimate atmospheric pressure.
//
// ZERO_DELAY_MS controls the delay between zeroing samples.
//
// Higher sample count = better zero estimate, but slower startup.
// ------------------------------------------------------------

const int ZERO_SAMPLES = 150;
const int ZERO_DELAY_MS = 13; // 13 ms is around 75 Hz timing range, matching the sensor data rate.

// ------------------------------------------------------------
// Sampling
// ------------------------------------------------------------
//
// SAMPLE_DELAY_MS controls how fast spirometerUpdate() samples.
//
// 13 ms is around 75 Hz timing range.
// This matches the LPS35HW_RATE_75_HZ setting.
// ------------------------------------------------------------

const int SAMPLE_DELAY_MS = 13;

// ------------------------------------------------------------
// Breath detection
// ------------------------------------------------------------
//
// The code uses filtered deltaP to decide when a breath starts
// and when it ends.
//
// A breath starts after filtered deltaP is above the start threshold
// for several consecutive samples.
//
// A breath ends after filtered deltaP is below the stop threshold
// for several consecutive samples.
//
// This prevents false starts/stops from noise.
// ------------------------------------------------------------

// Start
const float DP_START_THRESHOLD_PA = 5.0;
const int BREATH_START_COUNT_THRESHOLD = 3;

// Stop
const float DP_STOP_THRESHOLD_PA = 2.0;
const int BREATH_STOP_COUNT_THRESHOLD = 4;

// ------------------------------------------------------------
// Filtering
// ------------------------------------------------------------
//
// EMA_ALPHA controls the exponential moving average filter.
//
// lower alpha = smoother signal, slower response
// higher alpha = faster response, noisier signal
//
// MAX_DELTA_STEP_PA limits sudden one-sample jumps or drops.
// This helps prevent sensor spikes from causing false breath detection.
// ------------------------------------------------------------

const float EMA_ALPHA = 0.25;
const float MAX_DELTA_STEP_PA = 8.0;  // This is in Pascals. Adjust based on sensors noise and expected pressure changes during breathing.


// ============================================================
// STATE VARIABLES
// ============================================================
//
// These variables store the current state of the spirometer.
// Since this is a library, main.cpp does not directly touch these.
// main.cpp only calls the public getter functions.
// ============================================================

// Breath state
bool breathActive = false;

// Atmospheric pressure zero value
float patm_Pa = 0.0;

// Filter state
float filteredDeltaP_Pa = 0.0;
bool filterInitialized = false;

// Breath detection counters
int breathCount = 0;
int breathStopCount = 0;

// Current breath values
float flow_Lps = 0.0;
float volume_measured = 0.0;
float volume_final = 0.0;

// Constant values for Sqrt Functon
const float C0 = -0.7576;
const float C1 = 0.634;

// 1-minute test averaging values
float totalBreathVolume_L = 0.0;
int completedBreathCount = 0;

// Tracking for debugging/calibration
float maxDeltaP_Pa = 0.0;

// Timing
unsigned long startTime_us = 0;
unsigned long lastIntegrationTime_us = 0;

// Breath timing
float breathStartTime_s = 0.0;
float breathEndTime_s = 0.0;

// ============================================================
// READ PRESSURE
// ============================================================
//
// Adafruit library returns pressure in hPa.
// Convert hPa to Pa by multiplying by 100.
//
// Example:
// 1000 hPa = 100000 Pa
// ============================================================

float readUpstreamPa() {
  return upstreamSensor.readPressure() * 100.0;
}

// ============================================================
// ZERO SENSOR TO ATMOSPHERE
// ============================================================
//
// This function measures the current absolute pressure many times,
// averages it, and saves that value as atmospheric pressure.
//
// After zeroing:
//
// deltaP = current pressure - patm_Pa
//
// This is equivalent to treating the downstream side as atmosphere.
// ============================================================

void calibrateZero() {
  float sumP = 0.0;

  for (int i = 0; i < ZERO_SAMPLES; i++) {
    float upstream_Pa = readUpstreamPa();

    sumP += upstream_Pa;

    delay(ZERO_DELAY_MS);
  }

  patm_Pa = sumP / ZERO_SAMPLES;
  Serial.print("ZERO_PATM_PA,");
  Serial.println(patm_Pa, 2);

  // Reset breath detection state after zeroing
  breathActive = false;
  breathCount = 0;
  breathStopCount = 0;

  // Reset filter state after zeroing
  filteredDeltaP_Pa = 0.0;
  filterInitialized = false;
}

// ============================================================
// DELTA P FILTER
// ============================================================
//
// This function filters the raw pressure difference.
//
// It does two things:
//
// 1. Limits sudden one-sample jumps or drops.
// 2. Applies an exponential moving average.
//
// The returned value is the filtered/smoothed deltaP used for:
// - breath start detection
// - breath stop detection
// - flow calculation
// - volume integration
// ============================================================

float filterDeltaP(float rawDeltaP_Pa) {
  // First sample initializes the filter
  if (!filterInitialized) {
    filteredDeltaP_Pa = rawDeltaP_Pa;
    filterInitialized = true;
    return filteredDeltaP_Pa;
  }

  // Difference between new raw value and previous filtered value
  float step = rawDeltaP_Pa - filteredDeltaP_Pa;

  // Limit sudden one-sample jumps or drops
  if (step > MAX_DELTA_STEP_PA) {
    rawDeltaP_Pa = filteredDeltaP_Pa + MAX_DELTA_STEP_PA;
  } 
  else if (step < -MAX_DELTA_STEP_PA) {
    rawDeltaP_Pa = filteredDeltaP_Pa - MAX_DELTA_STEP_PA;
  }

  // Exponential moving average
  filteredDeltaP_Pa =
    EMA_ALPHA * rawDeltaP_Pa +
    (1.0 - EMA_ALPHA) * filteredDeltaP_Pa;

  return filteredDeltaP_Pa;
}

// ============================================================
// FLOW CALCULATION
// ============================================================
//
// Converts filtered deltaP into flow using the polynomial regression.
//
// x = filtered deltaP in Pa
// Q = flow in L/s
//
// If the polynomial returns a negative flow, clamp it to zero.
// This prevents bad values near zero pressure from subtracting volume.
// ============================================================

float calculateFlow_Lps(float deltaP_Pa) {
  float x = deltaP_Pa;

  float flow =
    C0 +
    C1 * sqrt(x);
  
  if (flow < 0.0) {
    flow = 0.0;
  }

  return flow;
}

// ============================================================
// VOLUME_FINAL CALCULATION
// ============================================================
//
// Converts measured volume into final volume using the power regression.
//
// x = measured volume in L
// V = final volume in L
//
// If the polynomial returns a negative volume, will be zero due to no y-intercept. 
// ============================================================

float calculateFinalVolume_L(float measuredVolume_L) {
  float x = measuredVolume_L;

  float volume_final =
    0.885 * pow(x, 0.57);

  return volume_final;
}

// ============================================================
// SPIROMETER BEGIN
// ============================================================
//
// main.cpp calls this once inside setup().
//
// It initializes SPI, starts the pressure sensor,
// sets the sensor data rate, enables ODR/9 sensor-side low-pass filtering,
// and zeros the sensor to atmosphere.
// ============================================================

void spirometerBegin() {
  // Start SPI bus
  SPI.begin();

  // Start pressure sensor on SPI
  if (!upstreamSensor.begin_SPI(UPSTREAM_CS_PIN, &SPI)) {
    Serial.println("ERROR: Upstream LPS sensor not found.");
    while (1) {
      delay(10);
    }
  }

  // Higher data rate
  upstreamSensor.setDataRate(LPS35HW_RATE_75_HZ);

  // Enables the low pass filter with ODR/9 bandwidth.
  // We are already filtering in software with filterDeltaP() as well. 
  upstreamSensor.enableLowPass(false);

  // Zero pressure sensor to atmosphere
  calibrateZero();
  Serial.println("SPIROMETER_ZERORED");
  Serial.print("ATMOSPHERIC_ZERO_PA,");
  Serial.println(patm_Pa, 2);


  // Initialize timing
  startTime_us = micros();
  lastIntegrationTime_us = startTime_us;
}

// ============================================================
// RESET TEST DATA
// ============================================================
//
// This clears the current 1-minute test data.
//
// Use this before starting a new breathing test.
//
// This does not re-initialize the sensor.
// It only clears measurement state, volume totals, counters,
// and filter state.
// ============================================================

void spirometerResetTest() {
  // Reset breath state
  breathActive = false;
  breathCount = 0;
  breathStopCount = 0;

  // Reset current breath values
  flow_Lps = 0.0;
  volume_measured = 0.0;

  // Reset test averaging values
  totalBreathVolume_L = 0.0;
  completedBreathCount = 0;

  // Reset tracking values
  maxDeltaP_Pa = 0.0;

  // Reset breath timing
  breathStartTime_s = 0.0;
  breathEndTime_s = 0.0;

  // Reset filter state
  filteredDeltaP_Pa = 0.0;
  filterInitialized = false;

  // Reset timing
  startTime_us = micros();
  lastIntegrationTime_us = startTime_us;
}

// ============================================================
// SPIROMETER UPDATE
// ============================================================
//
//
// main.cpp calls this repeatedly inside loop().
//
// This function:
// 1. Gets current time.
// 2. Reads pressure.
// 3. Calculates deltaP relative to atmosphere.
// 4. Filters deltaP.
// 5. Detects breath start.
// 6. Detects breath end.
// 7. Integrates volume only while breathActive == true.
// 8. Stores each completed breath volume silently.
//
// There is no per-breath Serial print here.
// main.cpp only prints the final average after 60 seconds.
// ============================================================

void spirometerUpdate() {
  // ------------------------------------------------------------
  // TIME
  // ------------------------------------------------------------
  //
  // now_us is the current microsecond timestamp.
  //
  // time_s is elapsed time since the test started.
  //
  // dt_s is the time since the previous update.
  // dt_s is used for volume integration:
  //
  // volume += flow * dt
  //
  // Since flow is in L/s and dt is in seconds,
  // the added volume is in liters.
  // ------------------------------------------------------------

  unsigned long now_us = micros();
  float time_s = (now_us - startTime_us) / 1000000.0;

  float dt_s = (now_us - lastIntegrationTime_us) / 1000000.0;
  lastIntegrationTime_us = now_us;

  // ------------------------------------------------------------
  // READ PRESSURE
  // ------------------------------------------------------------

  float p1_abs_Pa = readUpstreamPa();

  // ------------------------------------------------------------
  // FIND DELTA P
  // ------------------------------------------------------------
  //
  // Since this is the one-sensor version:
  //
  // deltaP = upstream pressure - atmospheric pressure
  //
  // This treats the outlet/downstream pressure as atmosphere.
  // ------------------------------------------------------------

  float rawDeltaP_Pa = p1_abs_Pa - patm_Pa;

  // ------------------------------------------------------------
  // EXHALE-ONLY TESTING - This may be subject to change!! 
  // ------------------------------------------------------------
  //
  // Negative values are ignored for now.
  // This means the code only integrates exhale direction flow.
  // ------------------------------------------------------------

  if (rawDeltaP_Pa < 0.0) {
    rawDeltaP_Pa = 0.0;
  }

  // ------------------------------------------------------------
  // FILTER DELTA P
  // ------------------------------------------------------------
  //
  // smoothDeltaP_Pa is the filtered deltaP returned by filterDeltaP().
  //
  // Keeping this name avoids conflicting with the global filter state
  // variable named filteredDeltaP_Pa.
  // ------------------------------------------------------------

  float smoothDeltaP_Pa = filterDeltaP(rawDeltaP_Pa);
  Serial.print("RAW_DP,");
  Serial.println(rawDeltaP_Pa, 2);

  // ------------------------------------------------------------
  // BREATH START DETECTION
  // ------------------------------------------------------------
  //
  // A breath starts only after filtered deltaP stays above
  // DP_START_THRESHOLD_PA for BREATH_START_COUNT_THRESHOLD samples.
  // ------------------------------------------------------------

  if (!breathActive) {
    if (smoothDeltaP_Pa > DP_START_THRESHOLD_PA) {
      breathCount++;
    } 
    else {
      breathCount = 0;
    }

    if (breathCount >= BREATH_START_COUNT_THRESHOLD) {
      Serial.println("BREATH_START");
      breathActive = true;
      breathStopCount = 0;

      // Reset integration timing at actual breath start.
      // This prevents the first volume step from including time
      // before the breath started.
      lastIntegrationTime_us = now_us;
      dt_s = 0.0;

      breathStartTime_s = time_s;
      maxDeltaP_Pa = 0.0;

      // Reset current breath volume at the start of each breath.
      volume_measured = 0.0;
      volume_final = 0.0;
    }
  }

  // ------------------------------------------------------------
  // BREATH STOP DETECTION
  // ------------------------------------------------------------
  //
  // A breath ends only after filtered deltaP stays below
  // DP_STOP_THRESHOLD_PA for BREATH_STOP_COUNT_THRESHOLD samples.
  //
  // When a breath ends, the library silently adds the completed
  // breath volume to the total volume and increments the breath count.
  // ------------------------------------------------------------

  if (breathActive) {
    if (smoothDeltaP_Pa < DP_STOP_THRESHOLD_PA) {
      breathStopCount++;
    } 
    else {
      breathStopCount = 0;
    }

    if (breathStopCount >= BREATH_STOP_COUNT_THRESHOLD) {
      breathActive = false;
      breathCount = 0;
      breathStopCount = 0;
      flow_Lps = 0.0;

      breathEndTime_s = time_s;

      // Power model for final volume relation from measured volume
      volume_final = calculateFinalVolume_L(volume_measured);
      
      Serial.print("BREATH_VOLUME_L,");
      Serial.println(volume_final, 3);

      // Store completed breath volume for test average
      totalBreathVolume_L += volume_final;
      completedBreathCount++;

      // Reset current breath volume so the next breath starts clean
      volume_measured = 0.0;
      volume_final = 0.0;

      return;
    }
  }

  // ------------------------------------------------------------
  // INTEGRATE VOLUME ONLY DURING BREATH
  // ------------------------------------------------------------
  //
  // Flow model:
  //
  // Q = K * deltaP
  //
  // Q is flow in L/s.
  // deltaP is filtered pressure difference in Pa.
  // K_LINEAR is your linear regression calibration constant.
  //
  // Volume integration:
  //
  // volume += flow * dt
  //
  // L/s * s = L
  // ------------------------------------------------------------

  if (breathActive) {
    if (smoothDeltaP_Pa > maxDeltaP_Pa) {
      maxDeltaP_Pa = smoothDeltaP_Pa; //storing max deltaP for each breath for debugging and calibration purposes
    }

    // Sqrt model for flow relation from pressure
    flow_Lps = calculateFlow_Lps(smoothDeltaP_Pa);
    
    volume_measured += flow_Lps * dt_s;

  } 

  else {
    flow_Lps = 0.0;
  }

  // ------------------------------------------------------------
  // SAMPLE DELAY
  // ------------------------------------------------------------
  //
  // Keeps the update rate near the intended sensor data rate.
  // ------------------------------------------------------------

  delay(SAMPLE_DELAY_MS);
}

// ============================================================
// GET AVERAGE BREATH VOLUME
// ============================================================
//
// Returns the average volume of completed breaths only.
//
// If no breaths have been completed, return 0.0 to avoid dividing by zero.
// ============================================================

float spirometerGetAverageBreathVolume_L() {
  if (completedBreathCount == 0) {
    return 0.0;
  }

  return totalBreathVolume_L / completedBreathCount;
}

// ============================================================
// GET BREATH COUNT
// ============================================================
//
// Returns how many complete breaths were detected.
// ============================================================

int spirometerGetBreathCount() {
  return completedBreathCount;
}