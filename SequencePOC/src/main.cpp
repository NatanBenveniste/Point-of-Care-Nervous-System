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


float val = 6.7;
void loop() {
  Serial.println("starting");
  screen.startScreen(val);
  delay(2000);
  screen.infoScreen(val);
  delay(2000);
  screen.baseHRVprog(val);
  delay(2000);
  screen.baseHRVresults(67, 67, val);
  delay(2000);
  screen.BPprog(val);
  delay(2000);
  screen.BPresults(120, 80,val);
  delay(2000);
  screen.bpStimProg(val);
  delay(2000);
  screen.bpStimResults(67, 67, val);
  delay(2000);
  screen.spStimProg(val);
  delay(2000);
  screen.spStimResults(67, 67, 0.67, val);
  delay(2000);
  screen.finalResults(67, 67, 67, 67, 67, 67, 67, 67, 67, val);
  delay(2000);

  screen.errorScreen(val);
  delay(2000);
  screen.stopScreen(val);

  // Serial.println("before clear");
  // screen.display.clearBuffer();
  // Serial.println("after clearBuffer");

  // screen.display.setCursor(20, 20);
  // screen.display.setTextColor(EPD_BLACK);
  // screen.display.setTextSize(2);
  // screen.display.print("TEST");

  // Serial.println("before display");
  // screen.display.display();
  // Serial.println("after display");
  
  
  while(1) {
    Serial.println("done");
    delay(10000);
  }
}