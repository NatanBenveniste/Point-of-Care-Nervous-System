#include <Arduino.h>
#include <SPI.h>
#include <motorhelper.h>
#include <displaymanager.h>
#include <heartrate.h>

MotorControl control;
DisplayManager screen;
HeartRateMonitor hrm;

#define chargePin 26
#define buttonPin1 27
#define buttonPin2 20

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(chargePin, INPUT);
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  hrm.init();

  SPI1.setSCK(10);
  SPI1.setTX(11);
  SPI1.setRX(12);
  SPI1.begin();

  screen.init();
  screen.display.setRotation(0);
  screen.display.setTextColor(EPD_BLACK);
  screen.display.setFont(&FreeSerif9pt7b);
}


void loop() {
  screen.display.clearBuffer();
  screen.display.setCursor(60, 100);
  screen.display.setTextSize(2);
  screen.display.println("67 HRV MONITOR");
  screen.display.setTextSize(1);
  screen.display.setCursor(60,120);
  screen.display.println("Press START to begin exam");
  screen.display.display();
  
  while(digitalRead(buttonPin1) == HIGH) {
    delay(50);
  }

  Serial.println("starting collect");
  hrm.timedCollect(30);
  Serial.println("finished collect");

  hrm.ptProcess();
  Serial.print("hr: ");
  Serial.println(hrm.hr);
  Serial.print("rmssd: ");
  Serial.println(hrm.rmssd);

  screen.display.clearBuffer();
  screen.display.setCursor(60, 100);
  screen.display.setTextSize(1);
  screen.display.print("Resting HR (BPM): ");
  screen.display.println(hrm.hr);
  screen.display.setCursor(60,115);
  screen.display.print("Resting HRV (RMSSD, MS): ");
  screen.display.println(hrm.rmssd);
  screen.display.display();
  
  
  while(1) {
    Serial.println("done");
    delay(10000);
  }
}