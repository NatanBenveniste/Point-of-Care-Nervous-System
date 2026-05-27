#include "displaymanager.h"

DisplayManager::DisplayManager()
    : display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, &SPI1)
{
}

void DisplayManager::init() {
    Serial.println("Starting");

    // enable display
    pinMode(EPD_ENA, OUTPUT);
    digitalWrite(EPD_ENA, HIGH);

    

    delay(100);

    Serial.print("Busy pin state: "); 
    Serial.println(digitalRead(EPD_BUSY));

    display.begin(THINKINK_TRICOLOR);

    Serial.println("Display begin done");
}

void DisplayManager::runDemo() {
    Serial.println("Banner demo");
    display.clearBuffer();
    display.setTextSize(3);
    display.setCursor((display.width() - 144) / 2, (display.height() - 24) / 2);
    display.setTextColor(EPD_BLACK);
    display.print("Tri");
    display.setTextColor(EPD_RED);
    display.print("Color");
    display.display();

    delay(15000);

    Serial.println("Color rectangle demo");
    display.clearBuffer();
    display.fillRect(display.width() / 3, 0, display.width() / 3,
                    display.height(), EPD_BLACK);
    display.fillRect((display.width() * 2) / 3, 0, display.width() / 3,
                    display.height(), EPD_RED);
    display.display();

    delay(15000);

    Serial.println("Text demo");
    // large block of text
    display.clearBuffer();
    display.setTextSize(1);
    testdrawtext(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
        "adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, "
        "fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor "
        "neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet "
        "ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a "
        "tortor imperdiet posuere. ",
        EPD_BLACK);
    display.display();

    delay(15000);

    Serial.println("Line demo");

    display.clearBuffer();
    for (int16_t i = 0; i < display.width(); i += 4) {
        display.drawLine(0, 0, i, display.height() - 1, EPD_BLACK);
    }

    for (int16_t i = 0; i < display.height(); i += 4) {
        display.drawLine(display.width() - 1, 0, 0, i, EPD_RED);
    }
    display.display();

    delay(15000);
}

void DisplayManager::testdrawtext(const char *text, uint16_t color) {
    display.setCursor(0, 0);
    display.setTextColor(color);
    display.setTextWrap(true);
    display.print(text);  
}

void DisplayManager::fillScreenTest(const char c) {
    Serial.println("Starting");
    display.clearBuffer();

    uint16_t color;

switch (c) {
        case 'b':
            color = EPD_BLACK;
            Serial.println("Filling BLACK");
            break;

        case 'w':
            color = EPD_WHITE;
            Serial.println("Filling WHITE");
            break;

        case 'r':
            color = EPD_RED;
            Serial.println("Filling RED");
            break;

        default:
            Serial.println("Unknown color, defaulting to BLACK");
            color = EPD_BLACK;
            break;
    }

    display.fillScreen(color);
    display.display();

    Serial.println("Done");
}