#include <Arduino.h>
#include <Adafruit_LPS35HW.h>
#pragma once

// define control pins
#define IN1 3
#define IN2 2
#define EEP 4
#define VALVE 1

// define sensor pins (SPI)
#define LPS_CS   5
#define LPS_SCK  28
#define LPS_MISO 16
#define LPS_MOSI 19

class CuffControl {
public:
    CuffControl();

    void pumpForward();
    void stopPump();

    void valveOpen();
    void valveClose();
    void valvePulse(int ms);

    void setupHardware();

    void serialControl();

    void runBPMeasurement();
    void runFiveMinuteHold();
    void fullDeflate();
};