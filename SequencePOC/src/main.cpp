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

  Serial.println("starting collect 1");
  hrm.timedCollect(30);
  hrm.ptProcess();

  Serial.flush();
  delay(50);
  Serial.println("sending data 1");
  Serial.flush();
  delay(10);

  sendECGPacket(
    hrm.ptECG.t.data(),  hrm.ptECG.t.size(),
    hrm.rawECG.val.data(), hrm.rawECG.val.size(),
    hrm.ptECG.val.data(), hrm.ptECG.val.size(),
    hrm.peakIdx.data(), hrm.peakIdx.size(),
    hrm.removedIntervals.data(), hrm.removedIntervals.size(),
    0.0f, 0.0f
  );

  Serial.flush();
  Serial.println("data sent");

  hrm.clearVecs();
  hrm.leadsOffCount = 0;
  hrm.removedRRCount = 0;
  delay(50);

  Serial.println("starting collect 2");
  hrm.timedCollect(30);
  Serial.println("finished data 2");
  hrm.ptProcess();

  auto [hr, rmssd] = hrm.hrStats(hrm.rrIntervals);
  hrm.hr = hr;
  hrm.rmssd = rmssd;

  Serial.print("Heart Rate (bpm): ");
  Serial.println(hrm.hr);
  Serial.print("RMSSD (ms): ");
  Serial.println(hrm.rmssd);

  Serial.flush();
  delay(50);
  Serial.println("sending data 2");
  Serial.flush();

  delay(10);

  sendECGPacket(
    hrm.ptECG.t.data(),  hrm.ptECG.t.size(),
    hrm.rawECG.val.data(), hrm.rawECG.val.size(),
    hrm.ptECG.val.data(), hrm.ptECG.val.size(),
    hrm.peakIdx.data(), hrm.peakIdx.size(),
    hrm.removedIntervals.data(), hrm.removedIntervals.size(),
    hrm.hr, hrm.rmssd
);
Serial.flush();
Serial.println("data sent");
  

  while(1){
    Serial.println("done");
    delay(5000);
  }

}