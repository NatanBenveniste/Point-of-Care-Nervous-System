#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

// ============================================================
// PIN SETUP
// ============================================================

const int UPSTREAM_CS_PIN = 17;

Adafruit_LPS35HW upstreamSensor = Adafruit_LPS35HW();

// ============================================================
// SETTINGS
// ============================================================

const int ZERO_SAMPLES = 150;
const int ZERO_DELAY_MS = 13;
const int SAMPLE_DELAY_MS = 13;     // about 75 Hz timing range

// Breath detection
const float DP_START_THRESHOLD_PA = 5.0;
const int BREATH_START_COUNT_THRESHOLD = 3;

const float DP_STOP_THRESHOLD_PA = 2.0;
const int BREATH_STOP_COUNT_THRESHOLD = 4;

// Filtering
const float EMA_ALPHA = 0.25;
const float MAX_DELTA_STEP_PA = 8.0;

// ------------------------------------------------------------
// Sqrt flow model
// ------------------------------------------------------------
//
// This uses your calibrated regression relationship:
//
// Q = 0.367 + 0.149x
//
// x is filtered deltaP in Pa.
// Q is flow in L/s.
//
// Volume is still calculated by:
//
// volume += flow * dt
//
// L/s * s = L
// ------------------------------------------------------------

const float C0 = -0.7576;
const float C1 = 0.634;

// ============================================================
// VARIABLES
// ============================================================

bool collecting = false;
bool breathActive = false;

float patm_Pa = 0.0;              // Atmospheric pressure from zeroing

float filteredDeltaP_Pa = 0.0;
bool filterInitialized = false;

int breathCount = 0;
int breathStopCount = 0;

float flow_Lps = 0.0;
float volume_L = 0.0;

float maxDeltaP_Pa = 0.0;

unsigned long startTime_us = 0;
unsigned long lastIntegrationTime_us = 0;

float breathStartTime_s = 0.0;
float breathEndTime_s = 0.0;

// ============================================================
// SENSOR READ FUNCTIONS
// ============================================================

float readUpstreamPa() {
  return upstreamSensor.readPressure() * 100.0;  // hPa to Pa
}

// ============================================================
// ZERO SENSOR TO ATMOSPHERE
// ============================================================

void calibrateZero() {
  float sumP = 0.0;

  for (int i = 0; i < ZERO_SAMPLES; i++) {
    float upstream_Pa = readUpstreamPa();
    sumP += upstream_Pa;

    delay(ZERO_DELAY_MS);
  }

  patm_Pa = sumP / ZERO_SAMPLES;

  breathActive = false;
  breathCount = 0;
  breathStopCount = 0;

  filteredDeltaP_Pa = 0.0;
  filterInitialized = false;
}

// ============================================================
// DELTA P FILTER
// ============================================================

float filterDeltaP(float rawDeltaP_Pa) {
  if (!filterInitialized) {
    filteredDeltaP_Pa = rawDeltaP_Pa;
    filterInitialized = true;
    return filteredDeltaP_Pa;
  }

  float step = rawDeltaP_Pa - filteredDeltaP_Pa;

  if (step > MAX_DELTA_STEP_PA) {
    rawDeltaP_Pa = filteredDeltaP_Pa + MAX_DELTA_STEP_PA;
  } 
  else if (step < -MAX_DELTA_STEP_PA) {
    rawDeltaP_Pa = filteredDeltaP_Pa - MAX_DELTA_STEP_PA;
  }

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
    C1 * x;
  
  if (flow < 0.0) {
    flow = 0.0;
  }

  return flow;
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  SPI.begin();

  if (!upstreamSensor.begin_SPI(UPSTREAM_CS_PIN, &SPI)) {
    Serial.println("ERROR: Upstream LPS sensor not found.");
    while (1) {
      delay(10);
    }
  }

  upstreamSensor.setDataRate(LPS35HW_RATE_75_HZ);

  // Keep this false unless you specifically want sensor-side filtering.
  upstreamSensor.enableLowPass(false); // false is ODR/9 bandwidth which is good for general use.  true is ODR/20 which is extra low bandwidth and more delay.

  Serial.println("READY");
  Serial.println("Press Enter to zero and start collection.");
  Serial.println("Press Enter again to stop collection.");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  // ------------------------------------------------------------
  // Start / stop collection with Enter
  // ------------------------------------------------------------
  if (Serial.available() > 0) {
    Serial.readStringUntil('\n');

    if (!collecting) {
      Serial.println("ZEROING...");
      calibrateZero();

      collecting = true;
      breathActive = false;
      breathCount = 0;
      breathStopCount = 0;

      flow_Lps = 0.0;
      volume_L = 0.0;
      maxDeltaP_Pa = 0.0;

      breathStartTime_s = 0.0;
      breathEndTime_s = 0.0;

      startTime_us = micros();
      lastIntegrationTime_us = startTime_us;

      Serial.println("START_DATA");
      Serial.println("time_s,p1_abs_Pa,deltaP_Pa,filteredDeltaP_Pa,flow_Lps,volume_L,breathActive");
    } 
    else {
      collecting = false;
      breathActive = false;
      breathCount = 0;
      breathStopCount = 0;
      flow_Lps = 0.0;

      Serial.println("STOP_COLLECTION");

      Serial.print("FINAL_VOLUME_L,");
      Serial.println(volume_L, 4);

      Serial.print("MAX_DELTA_P_PA,");
      Serial.println(maxDeltaP_Pa, 2);

      Serial.println("Press Enter to start another breath.");
    }
  }

  if (!collecting) {
    return;
  }

  // ------------------------------------------------------------
  // Time
  // ------------------------------------------------------------
  unsigned long now_us = micros();
  float time_s = (now_us - startTime_us) / 1000000.0;

  float dt_s = (now_us - lastIntegrationTime_us) / 1000000.0;
  lastIntegrationTime_us = now_us;

  // ------------------------------------------------------------
  // Read sensor
  // ------------------------------------------------------------
  float p1_abs_Pa = readUpstreamPa();

  // ------------------------------------------------------------
  // Calculate deltaP = P1 - Patm
  // ------------------------------------------------------------
  float rawDeltaP_Pa = p1_abs_Pa - patm_Pa;

  // Exhale-only testing
  if (rawDeltaP_Pa < 0.0) {
    rawDeltaP_Pa = 0.0;
  }

  // ------------------------------------------------------------
  // Filter deltaP
  // ------------------------------------------------------------
  float smoothDeltaP_Pa = filterDeltaP(rawDeltaP_Pa);

  // ------------------------------------------------------------
  // Breath start detection
  // ------------------------------------------------------------
  if (!breathActive) {
    if (smoothDeltaP_Pa > DP_START_THRESHOLD_PA) {
      breathCount++;
    } 
    else {
      breathCount = 0;
    }

    if (breathCount >= BREATH_START_COUNT_THRESHOLD) {
      breathActive = true;
      breathStopCount = 0;
      lastIntegrationTime_us = now_us;

      breathStartTime_s = time_s;
      maxDeltaP_Pa = 0.0;

      Serial.println("BREATH_START");
    }
  }

  // ------------------------------------------------------------
  // Breath stop detection
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
      collecting = false;
      breathCount = 0;
      breathStopCount = 0;
      flow_Lps = 0.0;

      breathEndTime_s = time_s;

      Serial.println("BREATH_END");

      Serial.print("FINAL_VOLUME_L,");
      Serial.println(volume_L, 4);

      Serial.print("BREATH_DURATION_S,");
      Serial.println(breathEndTime_s - breathStartTime_s, 4);

      Serial.print("MAX_DELTA_P_PA,");
      Serial.println(maxDeltaP_Pa, 2);

      Serial.println("STOP_COLLECTION");
      Serial.println("Press Enter to start another breath.");
      return;
    }
  }

  // ------------------------------------------------------------
  // INTEGRATE VOLUME ONLY DURING BREATH
  // ------------------------------------------------------------
  //
  // Flow model:
  //
  // Q = 0.367 + 0.149x - 1.03E-03x^2 + 4.34E-06x^3 - 6.63E-09x^4
  //
  // x is filtered pressure difference in Pa.
  // Q is flow in L/s.
  //
  // Volume integration:
  //
  // volume += flow * dt
  //
  // L/s * s = L
  // ------------------------------------------------------------

  if (breathActive) {
    if (smoothDeltaP_Pa > maxDeltaP_Pa) {
      maxDeltaP_Pa = smoothDeltaP_Pa;
    }

    flow_Lps = calculateFlow_Lps(smoothDeltaP_Pa);

    volume_L += flow_Lps * dt_s;
  } 
  else {
    flow_Lps = 0.0;
  }

  // ------------------------------------------------------------
  // Print only breath points
  // ------------------------------------------------------------
  if (breathActive) {
    Serial.print(time_s, 4);
    Serial.print(",");
    Serial.print(p1_abs_Pa, 2);
    Serial.print(",");
    Serial.print(rawDeltaP_Pa, 2);
    Serial.print(",");
    Serial.print(smoothDeltaP_Pa, 2);
    Serial.print(",");
    Serial.print(flow_Lps, 4);
    Serial.print(",");
    Serial.print(volume_L, 4);
    Serial.print(",");
    Serial.println(breathActive ? 1 : 0);
  }

  delay(SAMPLE_DELAY_MS);
}