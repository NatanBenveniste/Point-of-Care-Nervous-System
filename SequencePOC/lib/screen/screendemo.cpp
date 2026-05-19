// /***************************************************
//   Adafruit invests time and resources providing this open source code,
//   please support Adafruit and open-source hardware by purchasing
//   products from Adafruit!

//   Written by Limor Fried/Ladyada for Adafruit Industries.
//   MIT license, all text above must be included in any redistribution
//  ****************************************************/

// #include "Adafruit_ThinkInk.h"
// #define EPD_DC 15
// #define EPD_CS 9
// #define EPD_BUSY 12 // can set to -1 to not use a pin (will wait a fixed delay)
// #define SRAM_CS 14
// #define EPD_RESET 13  // can set to -1 and share with microcontroller Reset!
// #define EPD_SPI SPI // primary SPI

// // 3.7" Tricolor Display with 420x240 pixels and UC8253 chipset
// ThinkInk_370_Tricolor_BABMFGNR display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

// // Function declaration
// void testdrawtext(const char *text, uint16_t color);

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   Serial.println("Starting");

//   pinMode(EPD_BUSY, INPUT);

//   Serial.print("BUSY pin state: ");
//   Serial.println(digitalRead(EPD_BUSY));

//   display.begin(THINKINK_TRICOLOR);

//   Serial.println("Display begin done");
//   Serial.println("Free RAM test");
// }

// void loop() {
//   Serial.println("Banner demo");
//   display.clearBuffer();
//   display.setTextSize(3);
//   display.setCursor((display.width() - 144) / 2, (display.height() - 24) / 2);
//   display.setTextColor(EPD_BLACK);
//   display.print("Tri");
//   display.setTextColor(EPD_RED);
//   display.print("Color");
//   display.display();

//   delay(15000);

//   Serial.println("Color rectangle demo");
//   display.clearBuffer();
//   display.fillRect(display.width() / 3, 0, display.width() / 3,
//                    display.height(), EPD_BLACK);
//   display.fillRect((display.width() * 2) / 3, 0, display.width() / 3,
//                    display.height(), EPD_RED);
//   display.display();

//   delay(15000);

//   Serial.println("Text demo");
//   // large block of text
//   display.clearBuffer();
//   display.setTextSize(1);
//   testdrawtext(
//       "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
//       "adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, "
//       "fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor "
//       "neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet "
//       "ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a "
//       "tortor imperdiet posuere. ",
//       EPD_BLACK);
//   display.display();

//   delay(15000);

//   display.clearBuffer();
//   for (int16_t i = 0; i < display.width(); i += 4) {
//     display.drawLine(0, 0, i, display.height() - 1, EPD_BLACK);
//   }

//   for (int16_t i = 0; i < display.height(); i += 4) {
//     display.drawLine(display.width() - 1, 0, 0, i, EPD_RED);
//   }
//   display.display();

//   delay(15000);

  
//   // Serial.println("Drawing");

//   // display.clearBuffer();

//   // Serial.println("Buffer cleared");

//   // display.fillScreen(EPD_RED);

//   // display.display();

//   // Serial.println("Done");

//   // while (1);
// }

// void testdrawtext(const char *text, uint16_t color) {
//   display.setCursor(0, 0);
//   display.setTextColor(color);
//   display.setTextWrap(true);
//   display.print(text);
// }