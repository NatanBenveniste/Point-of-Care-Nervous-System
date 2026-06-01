// #include <Arduino.h>
// #include <SPI.h>
// #include <Adafruit_LPS35HW.h>
// #include <heartrate.h>

// Adafruit_LPS35HW sensor0;
// Adafruit_LPS35HW sensor1;

// HeartRateMonitor hrm;

// const int sensor0_cs = 17;
// // const int sensor1_cs = 9;



// void setup() {
// Serial.begin(115200);
//   delay(2000);

//   hrm.init();
// }

// void loop() {
//   delay(3000);
//   Serial.println("starting collect");
//   hrm.timedCollect(20);
//   Serial.println("finished collect");

//   hrm.ptECG = hrm.panTompkins(hrm.rawECG);
//   Serial.println("pan tompkins done");
//   hrm.bpECG = hrm.bandPass(hrm.rawECG);
//   Serial.println("bandpass done");
//   hrm.peakIdx = hrm.detectPeaks(hrm.ptECG, hrm.bpECG);
//   Serial.println("peak detection done, peak number: ");
//   Serial.println(hrm.peakIdx.size());
//   auto [hr, rmssd] = hrm.hrStats(hrm.ptECG, hrm.peakIdx);
//   Serial.println("heart rate stats done: ");
//   hrm.hr = hr;
//   hrm.rmssd = rmssd;
//   Serial.println(hr);
//   Serial.println(rmssd);

//   int n = hrm.ptECG.val.size();
//   Serial.println("sending data");
//   sendECGPacket(
//     hrm.ptECG.t.data(),  hrm.ptECG.t.size(),
//     hrm.rawECG.val.data(), hrm.rawECG.val.size(),
//     hrm.ptECG.val.data(), hrm.ptECG.val.size(),
//     hrm.peakIdx.data(), hrm.peakIdx.size(),
//     hrm.hr,
//     hrm.rmssd
// );
//   Serial.println("stop data");

  
//   while(1){
//     Serial.println("done");
//     delay(5000);
//   }
// }
