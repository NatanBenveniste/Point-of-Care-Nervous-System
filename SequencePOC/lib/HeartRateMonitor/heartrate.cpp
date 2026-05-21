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
int HeartRateMonitor::readECG() {
    if (HeartRateMonitor::leadsOff()) {
        return -1;
    }
    return (analogRead(ecgPin));
};

// set collecting flag true and set start time
void HeartRateMonitor::startCollecting() {
    collecting = true;
    startTime = std::chrono::steady_clock::now();
}

// add instantantaneous ecg reading to current raw signal vector
void HeartRateMonitor::updateRaw() {
    if(!collecting)
        return;

    auto now = std::chrono::steady_clock::now();
    double t = std::chrono::duration<double>(now - startTime).count();
    int v = readECG();

    rawECG.t.push_back(t);
    rawECG.val.push_back(v);
}

void HeartRateMonitor::panTompkins(const std::vector<double>& t, const std::vector<int>& val) {
    // to be called like panTompkins(rawECG.t, rawECG.val)
    // insert full algorithm
    const double fs = (t.back() - t.front())/(t.size()-1); // sampling frequency
    
    double f1 = 5 / fs; // cutoff low
    double f2 = 15 / fs; // cutoff high

    iirfilt_rrrf butter_bp = iirfilt_rrrf_create_prototype( // creating butterworth
        LIQUID_IIRDES_BUTTER,
        LIQUID_IIRDES_BANDPASS,
        LIQUID_IIRDES_SOS,
        2,
        f1,
        f2,
        0.0f,
        0.0f
    );

    std::vector<double> bpOut; // pre allocate bandpass output
    bpOut.reserve(val.size());

    for (double s : val) { // filter all of val, store in bpOut
        float filt;
        iirfilt_rrrf_execute(butter_bp, s, &filt);
        bpOut.push_back(filt);
    }

    iirfilt_rrrf_destroy(butter_bp);
    
}

