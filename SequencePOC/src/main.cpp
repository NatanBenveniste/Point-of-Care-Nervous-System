#include <core.h>
#include <motorhelper.h>
#include <displaymanager.h>
#include <heartrate.h>

MotorControl control;
DisplayManager screen;
HeartRateMonitor hrm;


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("serial");

  coreInit();
  hrm.init();
  screen.init();
}


void loop() {
  Serial.println("starting");
  // delay(2000);
  // screen.startScreen();
  // waitStart();
  // screen.infoScreen();
  // waitStart();
  // screen.baseHRVprog();
  // waitStart();
  // screen.baseHRVresults(67, 67);
  // waitStart();
  // screen.BPprog();
  // waitStart();
  // screen.BPresults(120, 80);
  // waitStart();
  // screen.bpStimProg();
  // waitStart();
  // screen.bpStimResults(67, 67);
  // waitStart();
  // screen.spStimProg();
  // waitStart();
  // screen.spStimResults(67, 67, 0.67);
  // waitStart();
  // screen.finalResults(67, 67, 67, 67, 67, 67, 67, 67, 67);

  Serial.println("before clear");
  screen.display.clearBuffer();
  Serial.println("after clearBuffer");

  screen.display.setCursor(20, 20);
  screen.display.setTextColor(EPD_BLACK);
  screen.display.setTextSize(2);
  screen.display.print("TEST");

  Serial.println("before display");
  screen.display.display();
  Serial.println("after display");
  
  
  while(1) {
    Serial.println("done");
    delay(10000);
  }
}