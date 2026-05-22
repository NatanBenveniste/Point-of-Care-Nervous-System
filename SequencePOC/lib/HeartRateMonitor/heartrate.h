#include <Arduino.h>
#include <math.h>
#include <vector>
#include <chrono>
#include <arm_math.h>



// define pins
#define leadOffPlus 21
#define leadOffMinus 22
#define ecgPin 28

struct ECG {
    std::vector<float> t;
    std::vector<float> val;
};

typedef struct {
    float b0, b1, b2;
    float a1, a2;
} biquad_t;

class HeartRateMonitor {
public: 
    HeartRateMonitor();

    // basic
    void init();    
    bool leadsOff();
    float readECG();

    // collecting
    bool collecting;
    std::chrono::steady_clock::time_point startTime;

    ECG rawECG;
    ECG ptECG;

    void startCollecting();
    void updateRaw();

    // processing
    ECG panTompkins(ECG ecg);
    
    std::vector<int> detectPeaks(std::vector<float>& signal, float fs);

private:    
    static inline void designBP(float fs, float f1, float f2, float coeffs[5]);

};