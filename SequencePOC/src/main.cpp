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

  hrm.ptECG = hrm.panTompkins(hrm.rawECG);
  Serial.println("pan tompkins done");
  hrm.bpECG = hrm.bandPass(hrm.rawECG);
  Serial.println("bandpass done");
  hrm.peakIdx = hrm.detectPeaks(hrm.ptECG, hrm.bpECG);
  Serial.println("peak detection done");
  auto [hr, rmssd] = hrm.hrStats(hrm.ptECG, hrm.peakIdx);
  Serial.println("heart rate stats done");
  hrm.hr = hr;
  hrm.rmssd = rmssd;

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
    delay(1000);
  }
}


