#include "heartrate.h"

HeartRateMonitor::HeartRateMonitor(){

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

// collect for a designated period, input: seconds
void HeartRateMonitor::timedCollect(int t) {
    int tMil = t * 1000;
    unsigned long start = millis();
    while (millis() - start < tMil){
        if (leadsOff()) {
            leadsOffCount++;
        }
        HeartRateMonitor::startCollecting();
        HeartRateMonitor::updateRaw();
        Serial.println("collecting. . ."); // comment out for clean serial monitor
        delay(4);
    }
    HeartRateMonitor::collecting = false;
}


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


static std::vector<int> findPeaks(const std::vector<float>& x, int distance, float height, float prominence) {
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

    std::vector<int> selected;

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

static std::vector<int> findPeaksAlt(const std::vector<float>& x,
                                        int distance,
                                        float height,
                                        float prominence)
{
    std::vector<int> peaks;

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

    std::vector<int> selected;

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
std::vector<int> HeartRateMonitor::detectPeaks(const ECG& ptECG, const ECG& bpECG) {
    const std::vector<float>& ptSignal = ptECG.val;
    const std::vector<float>& bpSignal = bpECG.val;
    const std::vector<float>& t = ptECG.t;
    const float fs = getFs(ptECG); // sampling frequency
    std::vector<int> finalPeaks;

    if (ptSignal.empty() || bpSignal.empty() || fs <= 0) {
        return finalPeaks;
    }

    int n = std::min(ptSignal.size(), bpSignal.size());

    int minDistance = std::max(1, (int)(0.35f * fs));
    int refractory = minDistance;
    int window = std::max(1, (int)(0.08f * fs));

    float height = percentile(ptSignal, 80.0f);

    std::vector<int> coarsePeaks = findPeaks(
        ptSignal,
        minDistance,
        height,
        0.015f
    );

    float SPKI = 0.0f, NPKI = 0.0f;
    float SPKF = 0.0f, NPKF = 0.0f;
    float TH1I = 0.0f, TH2I = 0.0f;
    float TH1F = 0.0f, TH2F = 0.0f;

    int initLen = std::min(n, (int)(2.0f * fs));

    float initMax = 0.0f;
    for (int i = 0; i < initLen; ++i) {
        initMax = std::max(initMax, ptSignal[i]);
    }

    SPKI = 0.6f * initMax;
    NPKI = 0.1f * initMax;

    float bpInitMax = 0.0f;
    for (int i = 0; i < initLen; ++i) {
        bpInitMax = std::max(bpInitMax, std::abs(bpSignal[i]));
    }

    SPKF = 0.6f * bpInitMax;
    NPKF = 0.1f * SPKF;

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

    std::set<int> searchbackDone;

    int lastPeak = std::numeric_limits<int>::min() / 2;

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

    std::vector<int> cleaned;
    for (int p : finalPeaks) {
        if (cleaned.empty() || p - cleaned.back() >= refractory) {
            cleaned.push_back(p);
        }
    }

    if (!cleaned.empty())
        cleaned.erase(cleaned.begin());

    if (!cleaned.empty())
        cleaned.pop_back();
    return cleaned;
}


// returns both hr(bpm) and rmssd(ms), input: pan tompkins ECG, peaks vector
std::pair<float, float> HeartRateMonitor::hrStats(const ECG& ptECG, const std::vector<int>& peaks) {
    const float fs = getFs(ptECG); // sampling frequency
    if (ptECG.t.size() < 2 || peaks.size() < 4 || fs <= 0.0f)
        return {0.0f, 0.0f};

    std::vector<float> rr_ms;

    for (size_t i = 1; i < peaks.size(); ++i) //  make vectors of rr intervals, initial absolute thresholding
    {
        float interval_ms = ((peaks[i] - peaks[i - 1]) / fs)* 1000.0f;

        if (interval_ms > 300.0f && interval_ms < 1500.0f)
            rr_ms.push_back(interval_ms);
    }
    
    float hr = 0.0f; // preallocate float outputs
    float rmssd = 0.0f;

    std::vector<float> filtered_rr_ms;
    if (!rr_ms.empty()) { // median thresholding (rr interval removal)
        std::vector<float> sorted_rr = rr_ms;
        std::sort(sorted_rr.begin(), sorted_rr.end());

        size_t n = sorted_rr.size();
        float med = (n % 2 == 0)
            ? (sorted_rr[n / 2 - 1] + sorted_rr[n / 2]) * 0.5f  // even num rr -> median = avg of two middle
            : sorted_rr[n / 2];  // odd num rr -> median = middle

        for (float r : rr_ms) {
            if (std::fabs(r - med) / med < 0.15f)
                filtered_rr_ms.push_back(r);
            else 
                intRemovedCount++;
            }

    }

    if (filtered_rr_ms.size() >= 3) {  // calculate hr + hrv
        std::vector<float> squared_diffs;

        hr = 60000.0f / meanVec(filtered_rr_ms);

        for (size_t i = 1; i < filtered_rr_ms.size(); ++i) {
            float d = filtered_rr_ms[i] - filtered_rr_ms[i - 1];
            squared_diffs.push_back(d * d);
        }

        rmssd = std::sqrt(meanVec(squared_diffs));
    }

    return {hr, rmssd};
}


// returns all results to class variables e.g. hrm.rmssd
void HeartRateMonitor::ptProcess() {
    HeartRateMonitor::ptECG = HeartRateMonitor::panTompkins(HeartRateMonitor::rawECG);
    HeartRateMonitor::bpECG = HeartRateMonitor::bandPass(HeartRateMonitor::rawECG);
    HeartRateMonitor::peakIdx = HeartRateMonitor::detectPeaks(HeartRateMonitor::ptECG, HeartRateMonitor::bpECG);
    auto [hr, rmssd] = HeartRateMonitor::hrStats(HeartRateMonitor::ptECG, HeartRateMonitor::peakIdx);
    HeartRateMonitor::hr = hr;
    HeartRateMonitor:: rmssd = rmssd;

}

void HeartRateMonitor::clearVecs() { 
    std::vector<float>().swap(HeartRateMonitor::rawECG.t);
    std::vector<float>().swap(HeartRateMonitor::rawECG.val);
    std::vector<float>().swap(HeartRateMonitor::bpECG.t);
    std::vector<float>().swap(HeartRateMonitor::bpECG.val);
    std::vector<float>().swap(HeartRateMonitor::ptECG.t);
    std::vector<float>().swap(HeartRateMonitor::ptECG.val);
}