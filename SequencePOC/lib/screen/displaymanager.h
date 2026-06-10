#pragma once

#include <Arduino.h>
#include "Adafruit_ThinkInk.h"
#include <string>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>

// control pins
#define EPD_DC 15
#define EPD_CS 13
#define EPD_BUSY 7 // usually 7
#define SRAM_CS 14 // usually 14
#define EPD_RESET 8
#define EPD_ENA 6

class DisplayManager {
public:
    DisplayManager();
    ThinkInk_420_Grayscale4_MFGN display;

    void init();

    // menu functions
    void startScreen(const float vBat);
    void infoScreen(const float vBat);

    void baseHRVprog(const float vBat);
    void baseHRVresults(const float hr, const float rmssd, const float vBat);

    void BPprog(const float vBat);
    void BPresults(const int SBP, const int DBP, const float vBat);

    void bpStimProg(const float vBat);
    void bpStimResults(const float hr, const float rmssd, const float vBat);

    void spStimProg(const float vBat);
    void spStimResults(const float hr, const float rmssd, const float fvc, const float vBat);

    void finalResults(
        const float rstHR, const float rstHRV,
        const int SBP, const int DBP,
        const float bpHR, const float bpHRV,
        const float spHR, const float spHRV,
        const float fvc,
        const float vBat
    );

    void stopScreen(const float vBat);
    void errorScreen(const float vBat);

private:
    // helper
    void centerText(const char *text, const int y);
    void drawHeader(const String &title);
    void drawBatteryVoltage(const float vBat);
};
