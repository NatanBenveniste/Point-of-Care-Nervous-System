/***************************************************
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_ThinkInk.h"
#include <SoftwareSPI.h>
#define EPD_DC 15
#define EPD_CS 17
#define EPD_BUSY 12 // can set to -1 to not use a pin (will wait a fixed delay)
#define SRAM_CS 14
#define EPD_RESET 13  // can set to -1 and share with microcontroller Reset!
#define EPD_SPI SPI // primary SPI

// #define SPI1_CLK 6
// #define SPI1_SDO 7
// #define SPI1_SDI 8

// SoftwareSPI soft_spi1(SPI1_CLK, SPI1_SDI, SPI1_SDO);


// 3.7" Tricolor Display with 420x240 pixels and UC8253 chipset
// ThinkInk_370_Tricolor_BABMFGNR display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, &soft_spi1);
ThinkInk_370_Tricolor_BABMFGNR display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

// Function declaration
void testdrawtext(const char *text, uint16_t color);

void setup() {
  Serial.begin(115200);

  SPI1.setSCK(6);
  SPI1.setTX(7);
  SPI1.setRX(8);
  SPI1.begin();
  Serial.println("SPI started");


  delay(1000);

  displayManager.init();

  delay(5000);
}

void loop() {
  displayManager.runDemo();
}