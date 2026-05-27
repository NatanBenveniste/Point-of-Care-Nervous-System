#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "Adafruit_ThinkInk.h"
#include <string>

// spi pins
#define EPD_SPI SPI1

// control pins
#define EPD_DC 15
#define EPD_CS 13
#define EPD_BUSY 7 // usually 7
#define SRAM_CS 14
#define EPD_RESET 8
#define EPD_ENA 6

class DisplayManager {
public:
    DisplayManager();

    void init();
    void runDemo();
    void fillScreenTest(const char c);

private:
    ThinkInk_370_Tricolor_BABMFGNR display;

    void testdrawtext(const char *text, uint16_t color);
};

#endif