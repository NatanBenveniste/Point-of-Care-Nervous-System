// #include <Arduino.h>
// #include "screen/displaymanager.h"
// #include "spirometer/sensor.h"

// #define SPI0_SCK 18
// #define SPI0_MISO 16
// #define SPI0_MOSI 19

// #define CS_UPSTREAM 17
// Adafruit_LPS35HW upstreamSensor;



// void setup() {
//   Serial.begin(115200);

//   delay(2000);
//   Serial.println("serial");

//   upstreamSensor.begin_SPI(CS_UPSTREAM, SPI0_SCK, SPI0_MISO, SPI0_MOSI);
//   // while (!Serial) {}

//   // if (!upstreamSensor.begin_SPI(CS_UPSTREAM)) {
//   //   Serial.println("UPSTREAM_SPI_INIT_FAILED");
//   //   while (1);
//   // }
//   Serial.println("time_ms,pressure_Pa");
//   delay(2000);
// }

// void loop() {
//   float upstreamPa = upstreamSensor.readPressure()*100;
//   Serial.print(millis());
//   Serial.print(",");
//   Serial.println(upstreamPa, 2);
//   delay(50);
// }



// #include <Wire.h>
// #include <SPI.h>
// #include <Adafruit_LPS35HW.h>

// #define LPS_CS 5

// Adafruit_LPS35HW lps35hw = Adafruit_LPS35HW();

// void setup() {
//   Serial.begin(115200);

//   delay(2000);
//   Serial.println("serial started");

//   if (!lps35hw.begin_SPI(LPS_CS, &SPI)) {
//     Serial.println("LPS33_SPI_INIT_FAILED");
//     while (1);
//   }
  
//   // lps35hw.begin_SPI(LPS_CS);

//   Serial.println("time_ms,pressure_Pa");
// }

// void loop() {
//   float pressure_Pa = lps35hw.readPressure() * 100.0;

//   Serial.print(millis());
//   Serial.print(",");
//   Serial.println(pressure_Pa, 2);

//   delay(50);
// }

#include <Arduino.h>
void setup() {
// initialize the serial communication:
Serial.begin(115200);
pinMode(21, INPUT); // Setup for leads off detection LO +
pinMode(22, INPUT); // Setup for leads off detection LO -
 
}
 
void loop() {
 
if((digitalRead(21) == 1)||(digitalRead(22) == 1)){
Serial.println('!');
}
else{
// send the value of analog input 0:
Serial.println(analogRead(28));
}
//Wait for a bit to keep serial data from saturating
delay(4);
}