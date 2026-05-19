#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "Adafruit_ThinkInk.h"

// spi pins
// #define EPD_SCK 6
// #define EPD_MOSI 7
// #define EPD_MISO 8
#define EPD_SPI SPI

// control pins
#define EPD_DC 15
#define EPD_CS 13
#define EPD_BUSY 7
#define SRAM_CS 14
#define EPD_RESET 8
#define EPD_ENA 6

class DisplayManager {
public:
    DisplayManager();

    void init();
    void runDemo();

private:
    ThinkInk_370_Tricolor_BABMFGNR display;

    void testdrawtext(const char *text, uint16_t color);
};

#endif