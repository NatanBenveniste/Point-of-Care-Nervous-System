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
  delay(3000);
  unsigned long start = millis();

  Serial.println("starting collect");
  hrm.timedCollect(30);
  Serial.println("finished collect");

  hrm.ptProcess();
  Serial.println(hrm.hr);
  Serial.println(hrm.rmssd);
  Serial.println(hrm.leadsOffCount);

  int n = hrm.ptECG.val.size();
  Serial.println("sending data");
  sendECGPacket(
    hrm.ptECG.t.data(),  hrm.ptECG.t.size(),
    hrm.rawECG.val.data(), hrm.rawECG.val.size(),
    hrm.ptECG.val.data(), hrm.ptECG.val.size(),
    hrm.peakIdx.data(), hrm.peakIdx.size(),
    hrm.hr,
    hrm.rmssd
);
  Serial.println("stop data");

  
  while(1){
    Serial.println("done");
    delay(5000);
  }

}