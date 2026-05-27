#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

// ============================================================
// SENSOR OBJECTS
// ============================================================
Adafruit_LPS35HW upstreamSensor;
Adafruit_LPS35HW downstreamSensor;

// Change these to match your wiring
const int UPSTREAM_CS_PIN = 5;
const int DOWNSTREAM_CS_PIN = 17;

// ============================================================
// SETTINGS
// ============================================================

// Zeroing
const int ZERO_SAMPLES = 100;
const int ZERO_DELAY_MS = 5;

// Breath detection ~ Form of processing to prevent false starts/stops from noise
// Start
const float DP_START_THRESHOLD_PA = 5.0;
const int BREATH_START_COUNT_THRESHOLD = 6;
// Stop 
const float DP_STOP_THRESHOLD_PA = 2.0;
const int BREATH_STOP_COUNT_THRESHOLD = 6;

const float DP_FLOW_CUTOFF_PA = 1.25;

// Filtering
const float EMA_ALPHA = 0.25;          // lower = smoother, higher = faster
const float MAX_DELTA_STEP_PA = 8.0;   // limits sudden one-sample jumps/drops
const float MAX_FLOW_LPS = 5.0;

// debugging hard cap for deltaP used in flow calc ~ prevents extreme outliers from dominating flow/volume calculations
// This is a temporary hack - will need to implement more robust outlier handling in the filter
const float MAX_FLOW_DP_PA = 15.0;   

// ============================================================
// STATE VARIABLES
// ============================================================

bool collecting = false;
bool breathActive = false;

float upstreamZero_Pa = 0.0;
float downstreamZero_Pa = 0.0;

int breathCount = 0;
int breathStopCount = 0;

float filteredDeltaP_Pa = 0.0;
bool filterInitialized = false;

// need to review timing 
unsigned long startTime_us = 0;
unsigned long lastIntegrationTime_us = 0;

// sqrt model derived values 
float volumeSqrt_L = 0.0;
float volumeLinear_L = 0.0;

// linear model derived values 
float flowSqrt_Lps = 0.0;
float flowLinear_Lps = 0.0;

// k constant for each model - actively being calibrated
float K_LINEAR = 0.09; // 1.0 / 12.48 = 0.08013 L/s per Pa ~ linear model constant
float K_SQRT = 0.85;     // sqrt model constant

// tracking for calibration 
float maxFilteredDeltaP_Pa = 0.0;
float breathStartTime_s = 0.0;
float breathEndTime_s = 0.0;


// ============================================================
// READ PRESSURE
// ============================================================
// Adafruit library returns pressure in hPa.
// Convert hPa to Pa by multiplying by 100.

float readPressurePa(Adafruit_LPS35HW &sensor) {
  return sensor.readPressure() * 100.0;
}

// ============================================================
// ZERO BOTH SENSORS
// ============================================================

void zeroBothSensors() {
  float upstreamSum = 0.0;
  float downstreamSum = 0.0;

  for (int i = 0; i < ZERO_SAMPLES; i++) {
    float upstreamPa = readPressurePa(upstreamSensor);
    float downstreamPa = readPressurePa(downstreamSensor);

    upstreamSum += upstreamPa;
    downstreamSum += downstreamPa;

    delay(ZERO_DELAY_MS);
  }

  upstreamZero_Pa = upstreamSum / ZERO_SAMPLES;
  downstreamZero_Pa = downstreamSum / ZERO_SAMPLES;

  breathCount = 0;
  breathActive = false;

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
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  SPI.begin();

  if (!upstreamSensor.begin_SPI(UPSTREAM_CS_PIN)) {
    Serial.println("ERROR: Upstream LPS sensor not found.");
    while (1) {
      delay(10);
    }
  }

  if (!downstreamSensor.begin_SPI(DOWNSTREAM_CS_PIN)) {
    Serial.println("ERROR: Downstream LPS sensor not found.");
    while (1) {
      delay(10);
    }
  }

  // Higher data rate
  upstreamSensor.setDataRate(LPS35HW_RATE_75_HZ);
  downstreamSensor.setDataRate(LPS35HW_RATE_75_HZ);

  Serial.println("READY");
  Serial.println("Press Enter to zero and start collection.");
  Serial.println("Press Enter again to stop collection.");
}

void loop() {
  // ------------------------------------------------------------
  // ENTER TO START / STOP
  // ------------------------------------------------------------
  if (Serial.available() > 0) {
    Serial.readStringUntil('\n');

    if (!collecting) {
      Serial.println("ZEROING...");
      zeroBothSensors();

      collecting = true;
      breathActive = false;
      breathCount = 0;
      breathStopCount = 0;

      volumeSqrt_L = 0.0;
      volumeLinear_L = 0.0;
      flowSqrt_Lps = 0.0;
      flowLinear_Lps = 0.0; 
      maxFilteredDeltaP_Pa = 0.0;
      breathStartTime_s = 0.0;
      breathEndTime_s = 0.0;

      startTime_us = micros();
      lastIntegrationTime_us = startTime_us;

      Serial.println("START_COLLECTION");
      Serial.println("time_s,upstreamRel_Pa,downstreamRel_Pa,rawDeltaP_Pa,filteredDeltaP_Pa,flowSqrt_Lps,volumeSqrt_L,flowLinear_Lps,volumeLinear_L,breathActive");    } 
   
      else {
      collecting = false;
      breathActive = false;
      breathCount = 0;
      breathStopCount = 0;

      Serial.println("STOP_COLLECTION");
      Serial.print("FINAL_VOLUME_L,");
      Serial.print("FINAL_VOLUME_SQRT_L,");
      Serial.println(volumeSqrt_L, 4);
      Serial.print("FINAL_VOLUME_LINEAR_L,");
      Serial.println(volumeLinear_L, 4);
      Serial.println("Press Enter to start another breath.");
    }
  }

  if (!collecting) {
    return;
  }

  // ------------------------------------------------------------
  // TIME
  // ------------------------------------------------------------
  unsigned long now_us = micros();
  float time_s = (now_us - startTime_us) / 1000000.0;

  float dt_s = (now_us - lastIntegrationTime_us) / 1000000.0;
  lastIntegrationTime_us = now_us;

  // ------------------------------------------------------------
  // READ PRESSURES
  // ------------------------------------------------------------
  float upstreamPa = readPressurePa(upstreamSensor);
  float downstreamPa = readPressurePa(downstreamSensor);

  // ------------------------------------------------------------
  // ZERO EACH SENSOR
  // ------------------------------------------------------------
  float upstreamRel_Pa = upstreamPa - upstreamZero_Pa;
  float downstreamRel_Pa = downstreamPa - downstreamZero_Pa;

  // ------------------------------------------------------------
  // FIND DELTA P
  // ------------------------------------------------------------
  float rawDeltaP_Pa = upstreamRel_Pa - downstreamRel_Pa;

  // For now: exhale-only testing
  if (rawDeltaP_Pa < 0.0) {
    rawDeltaP_Pa = 0.0;
  }

  // ------------------------------------------------------------
  // FILTER DELTA P
  // ------------------------------------------------------------
  float smoothDeltaP_Pa = filterDeltaP(rawDeltaP_Pa);

  // ------------------------------------------------------------
  // BREATH START DETECTION
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
      maxFilteredDeltaP_Pa = 0.0;

      Serial.println("BREATH_START");
    }
  }

// ------------------------------------------------------------
// BREATH STOP DETECTION
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
    flowSqrt_Lps = 0.0;
    flowLinear_Lps = 0.0; 
    breathEndTime_s = time_s;

    Serial.println("BREATH_END");

    Serial.print("FINAL_VOLUME_SQRT_L,");
    Serial.println(volumeSqrt_L, 4);

    Serial.print("FINAL_VOLUME_LINEAR_L,");
    Serial.println(volumeLinear_L, 4);

    Serial.print("BREATH_DURATION_S,");
    Serial.println(breathEndTime_s - breathStartTime_s, 4);

    Serial.print("MAX_FILTERED_DP_PA,");
    Serial.println(maxFilteredDeltaP_Pa, 2);

    Serial.println("STOP_COLLECTION");
    Serial.println("Press Enter to start another breath.");
    return;
  }
}

  // ------------------------------------------------------------
  // INTEGRATE VOLUME ONLY DURING BREATH
  // ------------------------------------------------------------
if (breathActive) {
  if (smoothDeltaP_Pa > maxFilteredDeltaP_Pa) {
    maxFilteredDeltaP_Pa = smoothDeltaP_Pa;
  }

  float dpForFlow_Pa = smoothDeltaP_Pa;

  if (dpForFlow_Pa > MAX_FLOW_DP_PA) {
    dpForFlow_Pa = MAX_FLOW_DP_PA;
  }

  flowSqrt_Lps = K_SQRT * sqrt(dpForFlow_Pa);
  flowLinear_Lps = K_LINEAR * dpForFlow_Pa;

  volumeSqrt_L += flowSqrt_Lps * dt_s;
  volumeLinear_L += flowLinear_Lps * dt_s;
}
 else {
  flowSqrt_Lps = 0.0;
  flowLinear_Lps = 0.0;
}

  // --- ---------------------------------------------------------
  // PRINT ONLY BREATH POINTS
  // ------------------------------------------------------------
  if (breathActive) {
    Serial.print(time_s, 4);
    Serial.print(",");
    Serial.print(upstreamRel_Pa, 2);
    Serial.print(",");
    Serial.print(downstreamRel_Pa, 2);
    Serial.print(",");
    Serial.print(rawDeltaP_Pa, 2);
    Serial.print(",");
    Serial.print(smoothDeltaP_Pa, 2);
    Serial.print(",");
    Serial.print(flowSqrt_Lps, 4);
    Serial.print(",");
    Serial.print(volumeSqrt_L, 4);
    Serial.print(",");
    Serial.print(flowLinear_Lps, 4);
    Serial.print(",");
    Serial.print(volumeLinear_L, 4);
    Serial.print(",");
    Serial.println(breathActive ? 1 : 0);
  }

  delay(10); // Attempting to read at ~100 Hz
}