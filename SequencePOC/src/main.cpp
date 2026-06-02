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

  coreInit();
  hrm.init();
  screen.init();
}


void loop() {
  delay(2000);
  screen.startScreen();
  waitStart();
  screen.infoScreen();
  waitStart();
  screen.baseHRVprog();
  waitStart();
  screen.baseHRVresults(67, 67);
  waitStart();
  screen.BPprog();
  waitStart();
  screen.BPresults(120, 80);
  waitStart();
  screen.bpStimProg();
  waitStart();
  screen.bpStimResults(67, 67);
  waitStart();
  screen.spStimProg();
  waitStart();
  screen.spStimResults(67, 67, 0.67);
  waitStart();
  screen.finalResults(67, 67, 67, 67, 67, 67, 67, 67, 67);


  



  // screen.display.clearBuffer();
  // screen.display.setCursor(60, 100);
  // screen.display.setTextSize(1);
  // screen.display.print("Resting HR (BPM): ");
  // screen.display.println(hrm.hr);
  // screen.display.setCursor(60,115);
  // screen.display.print("Resting HRV (RMSSD, MS): ");
  // screen.display.println(hrm.rmssd);
  // screen.display.display();
  
  
  while(1) {
    Serial.println("done");
    delay(10000);
  }
}