#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

// Motor driver pins
#define IN1 3
#define IN2 2
#define EEP 4

// Valve pin
#define VALVE 1

// LPS35HW SPI pins
#define LPS_CS   5
#define LPS_SCK  28
#define LPS_MISO 16
#define LPS_MOSI 19

class CuffControl {
public:
    CuffControl();

    void setupHardware();
    void serialControl();

    // 1) Blocking BP measurement
    void bpUpdate();

    // 2) Non-blocking 5-minute inflation/hold
    void bpInflate5Min();

    // 3) Non-blocking full deflation
    void bpDeflate();

    void pumpForward();
    void stopPump();

    void valveOpen();
    void valveClose();

private:
    Adafruit_LPS35HW sensor;
    float ambientPressure;

    float readPressure();
    float getPressureMmHg();

    static const int MAX_POINTS = 1200;

    float pressureData[MAX_POINTS];
    float oscData[MAX_POINTS];
    int dataCount;

    void resetBPData();
    void savePoint(float pressure);
    void calculateBP();

    const float SBP_RATIO = 0.55;
    const float DBP_RATIO = 0.70;

    // Non-blocking 5-minute hold state
    enum HoldState {
        HOLD_IDLE,
        HOLD_ZERO_DEFLATE,
        HOLD_ZERO_SETTLE,
        HOLD_INFLATE,
        HOLD_ACTIVE,
        HOLD_DEFLATE
    };

    HoldState holdState;

    unsigned long holdStateStart_ms;
    unsigned long holdStart_ms;

    bool deflating;

    void startHold5();
    void startDeflate();
    void stopAll();
};