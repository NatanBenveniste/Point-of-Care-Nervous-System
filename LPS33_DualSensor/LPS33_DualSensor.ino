#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

#define CS_UPSTREAM 9
#define CS_DOWNSTREAM 10

Adafruit_LPS35HW upstreamSensor;
Adafruit_LPS35HW downstreamSensor;

float zeroOffset_Pa = 0.0;

float readUpstreamPa() {
  return upstreamSensor.readPressure() * 100.0;
}

float readDownstreamPa() {
  return downstreamSensor.readPressure() * 100.0;
}

void setup() {
  Serial.begin(115200);

  if (!upstreamSensor.begin_SPI(CS_UPSTREAM)) {
    Serial.println("UPSTREAM_SENSOR_INIT_FAILED");
    while (1);
  }

  if (!downstreamSensor.begin_SPI(CS_DOWNSTREAM)) {
    Serial.println("DOWNSTREAM_SENSOR_INIT_FAILED");
    while (1);
  }

  delay(1000);

  float sum = 0.0;
  int samples = 100;

  for (int i = 0; i < samples; i++) {
    float upstream_Pa = readUpstreamPa();
    float downstream_Pa = readDownstreamPa();

    sum += upstream_Pa - downstream_Pa;
    delay(20);
  }

  zeroOffset_Pa = sum / samples;

  Serial.println("time_ms,upstream_Pa,downstream_Pa,deltaP_raw_Pa,deltaP_corrected_Pa");
}

void loop() {
  float upstream_Pa = readUpstreamPa();
  float downstream_Pa = readDownstreamPa();

  float deltaP_raw_Pa = upstream_Pa - downstream_Pa;
  float deltaP_corrected_Pa = deltaP_raw_Pa - zeroOffset_Pa;

  Serial.print(millis());
  Serial.print(",");
  Serial.print(upstream_Pa, 2);
  Serial.print(",");
  Serial.print(downstream_Pa, 2);
  Serial.print(",");
  Serial.print(deltaP_raw_Pa, 2);
  Serial.print(",");
  Serial.println(deltaP_corrected_Pa, 2);

  delay(50);
}