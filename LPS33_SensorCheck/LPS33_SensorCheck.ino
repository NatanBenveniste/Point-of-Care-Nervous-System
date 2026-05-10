#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

#define LPS_CS 10

Adafruit_LPS35HW lps35hw = Adafruit_LPS35HW();

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (!lps35hw.begin_SPI(LPS_CS)) {
    Serial.println("LPS33_SPI_INIT_FAILED");
    while (1);
  }

  Serial.println("time_ms,pressure_Pa");
}

void loop() {
  float pressure_Pa = lps35hw.readPressure() * 100.0;

  Serial.print(millis());
  Serial.print(",");
  Serial.println(pressure_Pa, 2);

  delay(50);
}