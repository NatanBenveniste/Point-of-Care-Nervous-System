#pragma once

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

    std::vector<int32_t> peakIdx;
    std::vector<int32_t> removedIntervals;
    std::vector<float> rrIntervals;

    float hr;
    float rmssd;

    // basic
    void init();    
    bool leadsOff() const;
    float readECG() const;

    // collecting
    bool collecting;
    bool hrvDone;
    uint32_t startTime;
    uint32_t windowStart;
    int leadsOffCount;
    int removedRRCount;
    int windowCount;
    int targetWindows;
    void startCollecting();
    void updateRaw();
    bool windowElapsed();
    void beginMeasurement();

    // processing
    ECG bandPass(const ECG& ecg);
    ECG panTompkins(const ECG& ecg);
    
    std::vector<int32_t> detectPeaks(const ECG& ptECG, const ECG& bpECG);
    void hrStats(const std::vector<float>& filtered_rr_ms);


    void ptProcess();
    void buildRR(const ECG& ptECG, const std::vector<int32_t>& peaks);
    void clearVecs();

private:    
    void designBP(float fs, float f1, float f2, float coeffs[5]);
    uint32_t lastSampleTime;
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

static void writeIntVector(const int32_t* data, uint32_t n) {
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

    const int32_t* peakIndices,
    uint32_t peakLen,

    const int32_t* removedIndices,
    uint32_t removedLen,

    float heartRate,
    float rmssd
)
{

    // Magic header so Python knows binary packet starts here
    Serial.write((const uint8_t*)"ECG1", 4);
    writeFloatVector(timeData, timeLen);
    writeFloatVector(rawECG, rawLen);
    writeFloatVector(processedECG, processedLen);
    writeIntVector(peakIndices, peakLen);
    writeIntVector(removedIndices, removedLen);
    writeFloat(heartRate);
    writeFloat(rmssd);
}