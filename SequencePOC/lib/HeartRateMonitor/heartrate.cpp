#include "heartrate.h"

HeartRateMonitor::HeartRateMonitor(){

}

// initialize
void HeartRateMonitor::init() {
    pinMode(leadOffMinus, INPUT);
    pinMode(leadOffPlus, INPUT);
    pinMode(ecgPin, INPUT);
}

// electrode leads no signal
bool HeartRateMonitor::leadsOff() {
    return(digitalRead(leadOffMinus) == HIGH || digitalRead(leadOffPlus) == HIGH);
}

// return instantaneous ecg reading
float HeartRateMonitor::readECG() {
    if (HeartRateMonitor::leadsOff()) {
        return -1;
    }
    int val = analogRead(ecgPin);
    return (val * 1.0);
};

// set collecting flag true and set start time
void HeartRateMonitor::startCollecting() {
    if (collecting)
        return;
    collecting = true;
    startTime = std::chrono::steady_clock::now();
}

// add instantantaneous ecg reading to current raw signal vector
void HeartRateMonitor::updateRaw() {
    if(!collecting)
        return;

    auto now = std::chrono::steady_clock::now();
    float t = std::chrono::duration<float>(now - startTime).count();
    float v = readECG();

    rawECG.t.push_back(t);
    rawECG.val.push_back(v);
}

ECG HeartRateMonitor::panTompkins(ECG ecg) {
    // to be called like panTompkins(rawECG.t, rawECG.val)
    std::vector<float> t = ecg.t;
    std::vector<float> val = ecg.val;
    const double fs = (t.back() - t.front())/(t.size()-1); // sampling frequency
    const int N = val.size(); // number of values in signal
    
    /// bandpass
    float f1 = 5; // cutoff low
    float f2 = 15; // cutoff high
    float coeffs[5]; // pre allocate
    float state[4];

    designBP(fs, f1, f2, coeffs); // butterworth coefficients
    
    arm_biquad_cascade_df2T_instance_f32 S; // create instance
    arm_biquad_cascade_df2T_init_f32(&S, 1, coeffs, state); // initialize filter
    std::vector<float> bpOut (N); // preallocate output vector
    arm_biquad_cascade_df2T_f32(&S, val.data(), bpOut.data(), N); // execute filter, output: bpOut

    /// derivative
    std::vector<float> derivOut(N); // preallocate output vector 
    for (size_t i = 2; i < N - 2; i++) {  // mathematically compute derivative, output: derivOut
        derivOut[i] = (bpOut[i - 2] + 2 * bpOut[i - 1] - 2 * bpOut[i+1] - bpOut[i + 2]) * 0.125f;
    }


    /// square
    std::vector<float> sqOut(N); // preallocate output vector
    for (size_t i = 0; i < N; i++) {
        sqOut[i] = bpOut[i] * bpOut[i];
    }

    /// integrator
    std::vector<float> intOut(N); // preallocate output vector

    uint32_t numTaps = static_cast<uint32_t>(0.15f * fs);
    if (numTaps < 1) numTaps = 1;
    if (numTaps > N) numTaps = N;
        
    std::vector<float> coeffsInt(numTaps, 1.0f / numTaps); // fir coeffs

    std::vector<float> stateInt(N + numTaps - 1, 0.0f); // fir state buffer

    arm_fir_instance_f32 SInt; // create integrator instance and initialize
    arm_fir_init_f32(&SInt, numTaps, coeffsInt.data(), stateInt.data(), N);
    arm_fir_f32(&SInt, sqOut.data(), intOut.data(), N);
        

    // trim first second of measurement (remove parsing disturbance)
    float trimTime = 1.0f;

    auto it = std::lower_bound(t.begin(), t.end(), trimTime);
    size_t idx = std::distance(t.begin(), it);
    t.erase(t.begin(), t.begin() + idx);
    intOut.erase(intOut.begin(), intOut.begin() + idx);
    for (auto& x : t)
        x -= trimTime;

    ptECG.t = t;
    ptECG.val = intOut;
    return ptECG;
}

// get coefficients for bandpass
void HeartRateMonitor::designBP(float fs, float f1, float f2, float coeffs[5]) {
    float w1 = tanf(M_PI * f1 / fs); // pre warp frequency
    float w2 = tanf(M_PI * f2 / fs);   
    
    float bw = w2 - w1; // bandwidth
    float w0 = sqrtf(w1*w2); // center frequency
    float T = 1.0f / fs; // sampling period
    float w02 = (w0 * w0);

    // butterworth coefficients
    float norm = 1.0f / (1.0f + bw + w02); // gain normalization factor

    float b0 = bw;
    float b1 = 0.0f;
    float b2 = -bw;
    float a1 = 2.0f * (w02 - 1.0f);
    float a2 = (1.0f - bw + w02);

    coeffs[0] = b0 * norm;
    coeffs[1] = b1 * norm;
    coeffs[2] = b2 * norm;
    coeffs[3] = -a1 * norm;
    coeffs[4] = -a2 * norm;
}

// peak detection
    std::vector<int> HeartRateMonitor::detectPeaks(std::vector<float>& signal, float fs) {
        
    }