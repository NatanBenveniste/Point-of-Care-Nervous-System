#pragma once

#include <Arduino.h>
#include "Adafruit_ThinkInk.h"
#include <string>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>

// spi pins
#define EPD_SPI SPI1

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
    ThinkInk_370_Tricolor_BABMFGNR display;

    void init();

    // menu functions
    void startScreen();
    void infoScreen();

    void baseHRVprog();
    void baseHRVresults(const float hr, const float rmssd);

    void BPprog();
    void BPresults(const float SBP, const float DBP);

    void bpStimProg();
    void bpStimResults(const float hr, const float rmssd);

    void spStimProg();
    void spStimResults(const float hr, const float rmssd);

    void finalResults();

private:
    void testdrawtext(const char *text, uint16_t color);

    // testing functions
    void runDemo();
    void fillScreenTest(const char c);
    void writeBig(const int x, const int y, const char *text);

};
