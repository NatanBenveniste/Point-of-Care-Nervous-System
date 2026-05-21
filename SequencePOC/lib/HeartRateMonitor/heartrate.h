#include <Arduino.h>
#include <math.h>
#include <vector>
#include <chrono>
extern "C" {
#include <liquid.h>
}



// define pins
#define leadOffPlus 21
#define leadOffMinus 22
#define ecgPin 28

struct ECG {
    std::vector<double> t;
    std::vector<int> val;
};

class HeartRateMonitor {
public: 
    HeartRateMonitor();

    // basic
    void init();    
    bool leadsOff();
    int readECG();

    // collecting
    bool collecting;
    std::chrono::steady_clock::time_point startTime;

    ECG rawECG;

    void startCollecting();
    void updateRaw();
    const std::vector<int>& getRawSignal() const; // needed ?


    // processing
    void panTompkins(const std::vector<double>& t, const std::vector<int>& val);

private:    

};