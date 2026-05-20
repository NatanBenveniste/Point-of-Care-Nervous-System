#include <Arduino.h>

// define pins
#define leadOffPlus 21
#define leadOffMinus 22
#define ecgPin 28

class HeartRateMonitor {
public: 
    HeartRateMonitor();

    void init();
    void readECG();
};