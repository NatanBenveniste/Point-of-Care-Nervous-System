#include <Arduino.h>
#include <heartrate.h>

HeartRateMonitor hrm;

void sendFloatVector(const float* data, uint32_t n) {
    // send length first
    Serial.write((uint8_t*)&n, sizeof(n));

    // send raw floats
    Serial.write((uint8_t*)data, n * sizeof(float));
}

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
  while (millis() - start < 20000){
    hrm.startCollecting();
    hrm.updateRaw();
    Serial.println("collecting. . .");
    delay(4);
  }
  hrm.collecting = false;
  Serial.println("finished collect");

  hrm.ptECG = hrm.panTompkins(hrm.rawECG);
  
  int n = hrm.ptECG.val.size();
  Serial.println("sending data");
  sendFloatVector(hrm.ptECG.t.data(), n);
  sendFloatVector(hrm.rawECG.val.data(), n);
  sendFloatVector(hrm.ptECG.val.data(), n);
  Serial.println("stop data");
  while(1){
    Serial.println("done");
    delay(1000);
  }


}


