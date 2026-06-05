#include "heartrate.h"
#include <set>
#include <numeric>
#include <limits>
#include <cmath>
HeartRateMonitor::HeartRateMonitor(){
    collecting = false;
    hr = 0.0f;
    rmssd = 0.0f;
    leadsOffCount = 0;
    removedRRCount = 0;
    windowCount = 0;
    targetWindows = 0;
    windowStart = 0;
    lastSampleTime = 0;
}
// -----------------------------------------------------------------------------------------------------
// basic + collection functions
// -----------------------------------------------------------------------------------------------------

// initialize
void HeartRateMonitor::init() {
    pinMode(leadOffMinus, INPUT);
    pinMode(leadOffPlus, INPUT);
    pinMode(ecgPin, INPUT);
}

// electrode leads no signal
bool HeartRateMonitor::leadsOff() const {
    return(digitalRead(leadOffMinus) == HIGH || digitalRead(leadOffPlus) == HIGH);
}

// return instantaneous ecg reading
float HeartRateMonitor::readECG() const {
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

    collecting = true; //Indicates that we are now collecting data
    startTime = micros();
    lastSampleTime = 0; // Allows update raw to collect immediately after startCollecting without waiting for 4ms
}

// add instantantaneous ecg reading to current raw signal vector
void HeartRateMonitor::updateRaw() {
    if(!collecting)
        return;

    uint32_t currentTime = micros();

    if (currentTime - lastSampleTime < 4000) // limit to ~250 Hz
        return;

    lastSampleTime = currentTime; 
    
    float t = (currentTime - startTime) / 1000000.0f;
    float v = readECG();

    rawECG.t.push_back(t);
    rawECG.val.push_back(v);
}

// collect for a designated period, input: seconds
//     int tMil = t * 1000;
//     unsigned long start = millis();
//     while (millis() - start < tMil){
//         if (leadsOff()) {
//             leadsOffCount++;
//         }
//         HeartRateMonitor::startCollecting();
//         HeartRateMonitor::updateRaw();
//         Serial.println("collecting. . ."); // comment out for clean serial monitor
//         delay(4);
//     }
//     HeartRateMonitor::collecting = false;
// }


// -----------------------------------------------------------------------------------------------------
// processing functions
// -----------------------------------------------------------------------------------------------------
// return sampling frequency (Hz)
float getFs(const ECG& ecg) {
     if (ecg.t.size() < 2)
        return 0.0f;

    float dt = (ecg.t.back() - ecg.t.front()) / (ecg.t.size() - 1);

    if (dt <= 0.0f)
        return 0.0f;
    return 1.0f / dt;
}

// full pan tompkins
ECG HeartRateMonitor::panTompkins(const ECG& ecg) {
    // pass in ecg SoA
    const std::vector<float>& t = ecg.t;
    const std::vector<float>& val = ecg.val;
    const float fs = getFs(ecg);                         // sampling frequency
    const int N = val.size();                            // number of values in signal
    
    if (fs <= 0.0f || N < 5 || t.size() != val.size())   // safety check
    return ECG{};

    /// bandpass
    float f1 = 5;                                        // cutoff low
    float f2 = 15;                                       // cutoff high
    float coeffs[5];                                     // pre allocate
    float state[4] = {0};

    designBP(fs, f1, f2, coeffs);                        // butterworth coefficients
    
    arm_biquad_cascade_df2T_instance_f32 S;              // create instance
    arm_biquad_cascade_df2T_init_f32(&S, 1, coeffs, state);    // initialize filter
    std::vector<float> bpOut (N);                        // preallocate output vector
    arm_biquad_cascade_df2T_f32(&S, val.data(), bpOut.data(), N); // execute filter, output: bpOut

    /// derivative + square
    std::vector<float> dsqOut(N);                      // preallocate output vector 
    for (size_t i = 2; i < N - 2; i++) {                 // mathematically compute derivative, output: derivOut
        
        float d = (bpOut[i - 2] + 2 * bpOut[i - 1] - 2 * bpOut[i+1] - bpOut[i + 2]) * 0.125f;
        dsqOut[i] = d * d;
    }
    std::vector<float>().swap(bpOut);                    // delete bandpass vector

    /// integrator
    std::vector<float> intOut(N);                        // preallocate output vector

    uint32_t numTaps = static_cast<uint32_t>(0.15f * fs);
    if (numTaps < 1) numTaps = 1;
    if (numTaps > N) numTaps = N;
        
    std::vector<float> coeffsInt(numTaps, 1.0f / numTaps); // fir coeffs

    std::vector<float> stateInt(N + numTaps - 1, 0.0f);  // fir state buffer

    arm_fir_instance_f32 SInt;                           // create integrator instance and initialize
    arm_fir_init_f32(&SInt, numTaps, coeffsInt.data(), stateInt.data(), N);
    arm_fir_f32(&SInt, dsqOut.data(), intOut.data(), N);
    std::vector<float>().swap(dsqOut);                    // delete square vector

    ECG out;
    out.t = t;
    out.val = std::move(intOut);
    return out;
}

// bandpass filter only
ECG HeartRateMonitor::bandPass(const ECG& ecg) {
    const std::vector<float>& t = ecg.t;
    const std::vector<float>& val = ecg.val;
    const float fs = getFs(ecg);                         // sampling frequency
    const int N = val.size();                            // number of values in signal

    if (fs <= 0.0f || N < 1 || t.size() != val.size())   // safety check
        return ECG{};
    
    /// bandpass
    float f1 = 5;                                        // cutoff low
    float f2 = 15;                                       // cutoff high
    float coeffs[5];                                     // pre allocate
    float state[4] = {0};

    designBP(fs, f1, f2, coeffs);                        // butterworth coefficients

    arm_biquad_cascade_df2T_instance_f32 S;              // create instance
    arm_biquad_cascade_df2T_init_f32(&S, 1, coeffs, state); // initialize filter
    std::vector<float> bpOut (N);                        // preallocate output vector
    arm_biquad_cascade_df2T_f32(&S, val.data(), bpOut.data(), N); // execute filter, output: bpOut

    ECG out;
    out.t = t;
    out.val = std::move(bpOut);
    return out;
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

    // normalize
    coeffs[0] = b0 * norm;
    coeffs[1] = b1 * norm;
    coeffs[2] = b2 * norm;
    coeffs[3] = -a1 * norm;
    coeffs[4] = -a2 * norm;
}

// trim data off start of ECG, input ECG + time to trim
// ECG HeartRateMonitor::trimECG(ECG ecg, float trimTime) {
//     std::vector<float> t = ecg.t;
//     std::vector<float> val = ecg.val;
//     ECG trimmedECG;

//     auto it = std::lower_bound(t.begin(), t.end(), trimTime);
//     size_t idx = std::distance(t.begin(), it);
//     t.erase(t.begin(), t.begin() + idx);
//     val.erase(val.begin(), val.begin() + idx);
//     for (auto& x : t)
//         x -= trimTime;

//     trimmedECG.t = t;
//     trimmedECG.val = val;
//     return trimmedECG;
// }

// -----------------------------------------------------------------------------------------------------
// peak detection
// -----------------------------------------------------------------------------------------------------
static float meanVec(const std::vector<float>& v) { // average of a vector
    if (v.empty()) return 0.0f;
    float s = std::accumulate(v.begin(), v.end(), 0.0f);
    return s / v.size();
}

static float percentile(std::vector<float> v, float pct) { // returns value at given percentile of range
    if (v.empty()) return 0.0f;
    std::sort(v.begin(), v.end());
    float idx = (pct / 100.0f) * (v.size() - 1);
    int lo = floor(idx);
    int hi = ceil(idx);
    float frac = idx - lo;
    return v[lo] * (1.0f - frac) + v[hi] * frac;
}


static std::vector<int32_t> findPeaks(const std::vector<float>& x, int distance, float height, float prominence) {
    struct Peak {
        int idx;
        float val;
    };

    std::vector<Peak> candidates;

    for (int i = 1; i < (int)x.size() - 1; ++i) {
        if (x[i] > x[i - 1] && x[i] >= x[i + 1] && x[i] >= height) {
            float leftMin = x[i];
            float rightMin = x[i];

            for (int j = i - 1; j >= 0; --j) {
                leftMin = std::min(leftMin, x[j]);
                if (x[j] > x[i]) break;
            }

            for (int j = i + 1; j < (int)x.size(); ++j) {
                rightMin = std::min(rightMin, x[j]);
                if (x[j] > x[i]) break;
            }

            float prom = x[i] - std::max(leftMin, rightMin);

            if (prom >= prominence) {
                candidates.push_back({i, x[i]});
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Peak& a, const Peak& b) {
                  return a.val > b.val;
              });

    std::vector<int32_t> selected;

    for (const auto& p : candidates) {
        bool tooClose = false;
        for (int q : selected) {
            if (std::abs(p.idx - q) < distance) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) selected.push_back(p.idx);
    }

    std::sort(selected.begin(), selected.end());
    return selected;
}

static std::vector<int32_t> findPeaksAlt(const std::vector<float>& x,
                                        int distance,
                                        float height,
                                        float prominence)
{
    std::vector<int32_t> peaks;

    if (x.size() < 3)
        return peaks;

    distance = std::max(distance, 1);

    struct Candidate {
        int idx;
        float val;
    };

    std::vector<Candidate> candidates;

    for (int i = 1; i < (int)x.size() - 1; ++i)
    {
        if (x[i] > x[i - 1] && x[i] >= x[i + 1] && x[i] >= height)
        {
            int left = std::max(0, i - distance);
            int right = std::min((int)x.size() - 1, i + distance);

            float localMin = x[i];

            for (int j = left; j <= right; ++j)
                localMin = std::min(localMin, x[j]);

            float prom = x[i] - localMin;

            if (prom >= prominence)
                candidates.push_back({i, x[i]});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.val > b.val;
              });

    std::vector<int32_t> selected;

    for (const auto& c : candidates)
    {
        bool tooClose = false;

        for (int p : selected)
        {
            if (std::abs(c.idx - p) < distance)
            {
                tooClose = true;
                break;
            }
        }

        if (!tooClose)
            selected.push_back(c.idx);
    }

    std::sort(selected.begin(), selected.end());
    return selected;
}

// returns vector indices of identified peaks, inputs: pan tompkins ecg, bandpass only ecg
std::vector<int32_t> HeartRateMonitor::detectPeaks(const ECG& ptECG, const ECG& bpECG) {
    const std::vector<float>& ptSignal = ptECG.val;
    const std::vector<float>& bpSignal = bpECG.val;
    const std::vector<float>& t = ptECG.t;
    const float fs = getFs(ptECG); // sampling frequency
    std::vector<int32_t> finalPeaks;

    if (ptSignal.empty() || bpSignal.empty() || fs <= 0) {
        return finalPeaks;
    }

    int n = std::min(ptSignal.size(), bpSignal.size());

    int minDistance = std::max(1, (int)(0.25f * fs));
    int refractory = minDistance;
    int window = std::max(1, (int)(0.08f * fs));

    //float height = percentile(ptSignal, 80.0f);
    float height = percentile(ptSignal, 35.0f);
    std::vector<int32_t> coarsePeaks = findPeaks(
        ptSignal,
        minDistance,
        height,
        0.015f
    );

    float SPKI = 0.0f, NPKI = 0.0f;
    float SPKF = 0.0f, NPKF = 0.0f;
    float TH1I = 0.0f, TH2I = 0.0f;
    float TH1F = 0.0f, TH2F = 0.0f;
int initStart = std::min(n - 1, (int)(2.0f * fs));
int initEnd   = std::min(n,     (int)(8.0f * fs));

std::vector<float> initPT;
std::vector<float> initBP;

for (int i = initStart; i < initEnd; ++i) {
    initPT.push_back(ptSignal[i]);
    initBP.push_back(std::abs(bpSignal[i]));
}

if (initPT.empty() || initBP.empty()) {
    return finalPeaks;
}

float initPTMed = percentile(initPT, 50.0f);
float initPT90  = percentile(initPT, 90.0f);

float initBPMed = percentile(initBP, 50.0f);
float initBP90  = percentile(initBP, 90.0f);

SPKI = initPT90;
NPKI = initPTMed;

SPKF = initBP90;
NPKF = initBPMed;
    // int initLen = std::min(n, (int)(2.0f * fs));

    // float initMax = 0.0f;
    // for (int i = 0; i < initLen; ++i) {
    //     initMax = std::max(initMax, ptSignal[i]);
    // }

    // SPKI = 0.6f * initMax;
    // NPKI = 0.1f * initMax;

    // float bpInitMax = 0.0f;
    // for (int i = 0; i < initLen; ++i) {
    //     bpInitMax = std::max(bpInitMax, std::abs(bpSignal[i]));
    // }

    // SPKF = 0.6f * bpInitMax;
    // NPKF = 0.1f * SPKF;

    TH1I = NPKI + 0.25f * (SPKI - NPKI);
    TH2I = 0.5f * TH1I;
    TH1F = NPKF + 0.25f * (SPKF - NPKF);
    TH2F = 0.5f * TH1F;

    std::vector<float> RR1;
    std::vector<float> RR2;

    float RR_AVG1 = 0.0f;
    float RR_AVG2 = 0.0f;

    const float LOW_LIM = 0.92f;
    const float HIGH_LIM = 1.16f;
    const float MISSED_LIM = 1.66f;

    auto updateRR = [&](int peakSample) {
        if (!finalPeaks.empty()) {
            float rr = ((float)(peakSample - finalPeaks.back()) / fs) * 1000.0f;

            RR1.push_back(rr);
            if (RR1.size() > 8) RR1.erase(RR1.begin());
            RR_AVG1 = meanVec(RR1);

            if (RR_AVG2 == 0.0f ||
                (LOW_LIM * RR_AVG2 <= rr && rr <= HIGH_LIM * RR_AVG2)) {
                RR2.push_back(rr);
                if (RR2.size() > 8) RR2.erase(RR2.begin());
                RR_AVG2 = meanVec(RR2);
            }
        }
    };

    std::set<int32_t> searchbackDone;

    int32_t lastPeak = std::numeric_limits<int32_t>::min() / 2;

    for (int i = 0; i < (int)coarsePeaks.size(); ++i) {
        int p = coarsePeaks[i];

        if (!finalPeaks.empty() && RR_AVG2 > 0.0f) {
            float elapsed = ((float)(p - finalPeaks.back()) / fs) * 1000.0f;

            if (elapsed > MISSED_LIM * RR_AVG2 &&
                searchbackDone.find(i) == searchbackDone.end()) {
                
                searchbackDone.insert(i);

                int lastIdx = 0;
                while (lastIdx < (int)coarsePeaks.size() &&
                       coarsePeaks[lastIdx] < finalPeaks.back()) {
                    lastIdx++;
                }

                for (int j = lastIdx + 1; j < i; ++j) {
                    int sp = coarsePeaks[j];

                    if (sp - finalPeaks.back() < refractory) {
                        continue;
                    }

                    float siVal = ptSignal[sp];

                    int sStart = std::max(0, sp - window);
                    int sEnd = std::min(n, sp + window);

                    int sfPeak = sStart;
                    float sfVal = bpSignal[sStart];

                    for (int k = sStart; k < sEnd; ++k) {
                        if (bpSignal[k] > sfVal) {
                            sfVal = bpSignal[k];
                            sfPeak = k;
                        }
                    }

                    if (siVal >= TH2I && sfVal >= TH2F) {
                        finalPeaks.push_back(sp);
                        updateRR(sp);

                        SPKI = 0.25f * siVal + 0.75f * SPKI;
                        SPKF = 0.25f * sfVal + 0.75f * SPKF;

                        TH1I = NPKI + 0.25f * (SPKI - NPKI);
                        TH2I = 0.5f * TH1I;
                        TH1F = NPKF + 0.25f * (SPKF - NPKF);
                        TH2F = 0.5f * TH1F;

                        break;
                    }
                }
            }
        }

        if (p - lastPeak < refractory) {
            continue;
        }

        float iVal = ptSignal[p];

        int start = std::max(0, p - window);
        int end = std::min(n, p + window);

        int fPeak = start;
        float fVal = std::abs(bpSignal[start]);

        for (int k = start; k < end; ++k) {
            if (bpSignal[k] > fVal) {
                fVal = bpSignal[k];
                fPeak = k;
            }
        }

        if (iVal >= TH1I && fVal >= TH1F) {
            finalPeaks.push_back(p);
            lastPeak = p;

            updateRR(p);

            SPKI = 0.125f * iVal + 0.875f * SPKI;
            SPKF = 0.125f * fVal + 0.875f * SPKF;
        } else {
            NPKI = 0.125f * iVal + 0.875f * NPKI;
            NPKF = 0.125f * fVal + 0.875f * NPKF;
        }

        TH1I = NPKI + 0.25f * (SPKI - NPKI);
        TH2I = 0.5f * TH1I;
        TH1F = NPKF + 0.25f * (SPKF - NPKF);
        TH2F = 0.5f * TH1F;
    }

    std::sort(finalPeaks.begin(), finalPeaks.end());
    finalPeaks.erase(std::unique(finalPeaks.begin(), finalPeaks.end()), finalPeaks.end());

    std::vector<int32_t> cleaned;
    for (int32_t p : finalPeaks) {
        if (cleaned.empty() || p - cleaned.back() >= refractory) {
            cleaned.push_back(p);
        }
    }

//    if (!cleaned.empty())
//         cleaned.erase(cleaned.begin());

//     if (!cleaned.empty())
//         cleaned.pop_back();
    return cleaned;
}


// returns both hr(bpm) and rmssd(ms), input: pan tompkins ECG, peaks vector
void HeartRateMonitor::buildRR(const ECG& ptECG, const std::vector<int32_t>& peaks) {
    const float fs = getFs(ptECG); // sampling frequency
    if (ptECG.t.size() < 2 || peaks.size() < 4 || fs <= 0.0f)
        return;
    
    int startIdx = 0;
    while (startIdx < (int)peaks.size() - 1 && ptECG.t[peaks[startIdx]] < 2.0f) {
        startIdx++;
    }

    std::vector<float> rr_ms;
    rr_ms.reserve(peaks.size()); // reserve space to avoid reallocations

    for (size_t i = startIdx + 1; i < peaks.size(); ++i) //  make vectors of rr intervals, initial absolute thresholding
    {
        float interval_ms =
    (ptECG.t[peaks[i]] - ptECG.t[peaks[i - 1]]) * 1000.0f;
        //float interval_ms = ((peaks[i] - peaks[i - 1]) / fs)* 1000.0f;

        if (interval_ms > 300.0f && interval_ms < 1500.0f)
            rr_ms.push_back(interval_ms);
    }
    
    if (rr_ms.size() < 3) {
        return; // not enough valid RR intervals to compute stats
    }

    // Step 1: median RR
    std::vector<float> sorted_rr = rr_ms;
    std::sort(sorted_rr.begin(), sorted_rr.end());

    size_t n = sorted_rr.size();
    float med = (n % 2 == 0)
        ? (sorted_rr[n / 2 - 1] + sorted_rr[n / 2]) * 0.5f
        : sorted_rr[n / 2];

    // Step 2: remove intervals far from median
    removedRRCount = 0;
    removedIntervals.clear();  // IMPORTANT: add this before loop

    for (size_t i = 0; i < rr_ms.size(); i++) {
        float r = rr_ms[i];
        float dev = std::fabs(r - med) / med;

        if (dev < 0.25f) {
            rrIntervals.push_back(r);
        } else {
            removedRRCount++;
            removedIntervals.push_back((int)i);  // store RR index
        }
    }
    Serial.print("Removed count: ");
    Serial.println(removedRRCount);
    Serial.print("Removed indices: ");
    for (int idx : removedIntervals) {
        Serial.print(idx);
        Serial.print(" ");
    }
    Serial.println();
}

void HeartRateMonitor::hrStats(const std::vector<float>& filtered_rr_ms) {
    if (filtered_rr_ms.size() < 3) {  // calculate hr + hrv
        hr = 0.0f;
        rmssd = 0.0f;
        return;
    }
    
    hr = 0.0f;
    rmssd = 0.0f;

    // HR
    float sum = std::accumulate(filtered_rr_ms.begin(), filtered_rr_ms.end(), 0.0f);
    float mean_rr = sum / filtered_rr_ms.size();
    hr = 60000.0f / mean_rr;

    // RMSSD
    float sq_sum = 0.0f;
    for (size_t i = 1; i < filtered_rr_ms.size(); i++) {
        float diff = filtered_rr_ms[i] - filtered_rr_ms[i - 1];
        sq_sum += diff * diff;
    }
    rmssd = std::sqrt(sq_sum / (filtered_rr_ms.size() - 1));

    std::vector<float>().swap(rrIntervals); // clear RR intervals for next window

 }


// returns all results to class variables e.g. hrm.rmssd
void HeartRateMonitor::ptProcess() {
    bpECG = bandPass(rawECG);
    ptECG = panTompkins(rawECG);
    peakIdx = detectPeaks(ptECG, bpECG);
    buildRR(ptECG, peakIdx);
}

void HeartRateMonitor::clearVecs() { 
    std::vector<float>().swap(HeartRateMonitor::rawECG.t);
    std::vector<float>().swap(HeartRateMonitor::rawECG.val);
    std::vector<float>().swap(HeartRateMonitor::bpECG.t);
    std::vector<float>().swap(HeartRateMonitor::bpECG.val);
    std::vector<float>().swap(HeartRateMonitor::ptECG.t);
    std::vector<float>().swap(HeartRateMonitor::ptECG.val);
    std::vector<int32_t>().swap(HeartRateMonitor::peakIdx);
}

void HeartRateMonitor::beginMeasurement(int seconds) {
    rrIntervals.clear();
    leadsOffCount = 0;
    removedRRCount = 0;
    removedIntervals.clear();

    hr = 0.0f;
    rmssd = 0.0f;

    windowCount = 0;
    targetWindows = seconds / 30;

    if (targetWindows < 1) {
        targetWindows = 1;
    }

    startTime = micros();
    windowStart = startTime;
    lastSampleTime = 0; 

    clearVecs();

    collecting = true;
}

bool HeartRateMonitor::windowElapsed() {
     if (!collecting) {
        return false;
    }

    return (micros() - windowStart) >= 30000000UL;
}

