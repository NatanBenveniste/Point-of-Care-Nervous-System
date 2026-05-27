#include <Arduino.h>
#include <math.h>
#include <vector>
#include <chrono>
#include <arm_math.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>
#include <set>
#include <cstdint>



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

    // variables
    ECG rawECG;
    ECG bpECG;
    ECG ptECG;
    std::vector<int> peakIdx;
    float hr;
    float rmssd;

    // basic
    void init();    
    bool leadsOff();
    float readECG();

    // collecting
    bool collecting;
    std::chrono::steady_clock::time_point startTime;
    void timedCollect(int t);

    void startCollecting();
    void updateRaw();

    // processing
    ECG bandPass(const ECG& ecg);
    ECG panTompkins(const ECG& ecg);
    // ECG trimECG(ECG ecg, float trimTime);
    
    std::vector<int> detectPeaks(const ECG& ptECG, const ECG& bpECG);
    std::pair<float, float> hrStats(const ECG& ptECG, const std::vector<int>& peaks);

    void ptProcess();
    void clearVecs();


private:    
    static inline void designBP(float fs, float f1, float f2, float coeffs[5]);

};



// serial package sender for ECG data
static void writeU32(uint32_t x) {
    Serial.write(reinterpret_cast<uint8_t*>(&x), sizeof(uint32_t));
}

static void writeFloat(float x) {
    Serial.write(reinterpret_cast<uint8_t*>(&x), sizeof(float));
}

static void writeFloatVector(const float* data, uint32_t n) {
    writeU32(n);
    Serial.write(reinterpret_cast<const uint8_t*>(data), n * sizeof(float));
}

static void writeIntVector(const int* data, uint32_t n) {
    writeU32(n);
    Serial.write(reinterpret_cast<const uint8_t*>(data), n * sizeof(int));
}

static void sendECGPacket(
    const float* timeData,
    uint32_t timeLen,

    const float* rawECG,
    uint32_t rawLen,

    const float* processedECG,
    uint32_t processedLen,

    const int* peakIndices,
    uint32_t peakLen,

    float heartRate,
    float rmssd
) {
    Serial.println("sending data");

    // Magic header so Python knows binary packet starts here
    Serial.write((const uint8_t*)"ECG1", 4);

    writeFloatVector(timeData, timeLen);
    writeFloatVector(rawECG, rawLen);
    writeFloatVector(processedECG, processedLen);
    writeIntVector(peakIndices, peakLen);

    writeFloat(heartRate);
    writeFloat(rmssd);
}