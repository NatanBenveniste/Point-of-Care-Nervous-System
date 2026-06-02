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

// ============================================================
// VARIABLES
// ============================================================

bool collecting = false;

float patm_Pa = 0.0;              // Atmospheric pressure from zeroing
float maxDeltaP_Pa = 0.0;         // Max deltaP = P1 - Patm

float sumDeltaP_Pa = 0.0;         // Sum of positive deltaP values
int deltaPCount = 0;              // Number of positive deltaP samples

unsigned long startTime_ms = 0;

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
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

//   SPI1.setSCK(10);
//   SPI1.setTX(11);
//   SPI1.setRX(12);
  SPI.begin();

  if (!upstreamSensor.begin_SPI(UPSTREAM_CS_PIN, &SPI)) {
    Serial.println("ERROR: Upstream LPS sensor not found.");
    while (1) {
      delay(10);
    }
  }

  upstreamSensor.setDataRate(LPS35HW_RATE_75_HZ);

  // Keep this false unless you specifically want sensor-side filtering.
  // Filtering can clean noise but adds delay.
  upstreamSensor.enableLowPass(false);

  Serial.println("READY");
  Serial.println("Press Enter to start.");
  Serial.println("The code will zero to Patm, then begin logging.");
  Serial.println("Press Enter again to stop.");
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

    collecting = !collecting;

    if (collecting) {
      Serial.println("ZEROING...");
      calibrateZero();

      maxDeltaP_Pa = 0.0;
      sumDeltaP_Pa = 0.0;
      deltaPCount = 0;

      startTime_ms = millis();

      Serial.println("START_DATA");
      Serial.println("time_ms,p1_abs_Pa,deltaP_Pa");
    } 
    else {
      Serial.println("END_DATA");

      Serial.print("MAX_DELTA_P_PA,");
      Serial.println(maxDeltaP_Pa, 2);

      Serial.print("AVG_DELTA_P_PA,");

      if (deltaPCount > 0) {
        float avgDeltaP_Pa = sumDeltaP_Pa / deltaPCount;
        Serial.println(avgDeltaP_Pa, 2);
      } 
      else {
        Serial.println(0.00, 2);
      }

      Serial.println("Press Enter to start another test.");
    }
  }

  if (!collecting) {
    return;
  }

  // ------------------------------------------------------------
  // Read sensor
  // ------------------------------------------------------------
  float p1_abs_Pa = readUpstreamPa();

  // ------------------------------------------------------------
  // Calculate deltaP = P1 - Patm
  // ------------------------------------------------------------
  float deltaP_Pa = p1_abs_Pa - patm_Pa;

  // ------------------------------------------------------------
  // Track max positive deltaP
  // ------------------------------------------------------------
  if (deltaP_Pa > maxDeltaP_Pa) {
    maxDeltaP_Pa = deltaP_Pa;
  }

  // ------------------------------------------------------------
  // Track average positive deltaP only
  // ------------------------------------------------------------
  if (deltaP_Pa > 0.0) {
    sumDeltaP_Pa += deltaP_Pa;
    deltaPCount++;
  }

  unsigned long time_ms = millis() - startTime_ms;

  // ------------------------------------------------------------
  // Print live CSV data
  // ------------------------------------------------------------
  Serial.print(time_ms);
  Serial.print(",");
  Serial.print(p1_abs_Pa, 2);
  Serial.print(",");
  Serial.println(deltaP_Pa, 2);

  delay(SAMPLE_DELAY_MS);
}