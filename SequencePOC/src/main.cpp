#include <Arduino.h>
#include <heartrate.h>

HeartRateMonitor hrm;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("serial");

  hrm.init();
}

void loop() {
  
  hrm.hrvStartCollecting(); // TODO 
  hrm.panTompkins(hrm.rawECG);
  hrm.clearVecs();

  auto [hr, rmssd] = hrm.hrStats(hrm.rrIntervals);
  hrm.hr = hr;
  hrm.rmssd = rmssd;

}