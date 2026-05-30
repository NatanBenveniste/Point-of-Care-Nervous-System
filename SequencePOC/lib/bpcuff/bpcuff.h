#pragma once

#include <Arduino.h>
#include <Adafruit_LPS35HW.h>

// define control pins
#define IN1 3
#define IN2 2
#define EEP 4
#define VALVE 1

// define sensor pins SPI
#define LPS_CS   5
#define LPS_SCK  28
#define LPS_MISO 16
#define LPS_MOSI 19

class CuffControl {
public:
    CuffControl();

    void setupHardware();

    void pumpForward();
    void stopPump();

    void valveOpen();
    void valveClose();

    void serialControl();

    void runBPMeasurement();
    void runFiveMinuteHold();
    void fullDeflate();

private:
    Adafruit_LPS35HW sensor;
    float ambientPressure;

    float readPressure();
    float getPressureMmHg();

    static const int MAX_POINTS = 300;

    float pressureData[MAX_POINTS];
    float oscData[MAX_POINTS];
    int dataCount;

    void resetBPData();
    void savePoint(float pressure, float osc);
    void calculateBP();

    const float SBP_RATIO = 0.20;
    const float DBP_RATIO = 0.45;
};