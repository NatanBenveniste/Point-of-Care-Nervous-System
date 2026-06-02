#include "displaymanager.h"

DisplayManager::DisplayManager()
    : display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, &SPI1)
{
}

void DisplayManager::init() {
    // enable display
    pinMode(EPD_ENA, OUTPUT);
    digitalWrite(EPD_ENA, HIGH);

    delay(100);

    // Serial.print("Busy pin state: "); 
    // Serial.println(digitalRead(EPD_BUSY));

    display.begin(THINKINK_TRICOLOR);
    display.setRotation(0); // 0 for flat, 2 for in device
    display.setTextColor(EPD_BLACK);
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

void DisplayManager::writeBig(const int x, const int y, const char *text) {
    display.clearBuffer();  
    display.setCursor(x, y);
    display.setTextSize(2);
    display.println(text);

    display.display();
}

void DisplayManager::fontTest() {
    display.clearBuffer();
    display.setCursor(0,20);
    display.setFont(&FreeSerif9pt7b);
    display.print("test ");
    display.setFont(&FreeSerif12pt7b);
    display.println("test ");
    display.setFont(&FreeSerif18pt7b);
    display.print("test ");
    display.setFont(&FreeSerif24pt7b);
    display.print("test ");
    display.display();
}

void DisplayManager::centerText(const String &text, const int y) {
    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (display.width() - w)/2;
    display.setCursor(x, y);
    display.print(text);
}


// menu functions
void DisplayManager::startScreen() {
    display.clearBuffer();
    display.setFont(&FreeSerif9pt7b);
    display.setTextSize(2);
    centerText("67 HRV MONITOR", 100);
    display.setTextSize(1);
    display.setFont(&FreeSerif12pt7b);
    centerText("Press START to begin exam", 120);
    display.display();
}


void DisplayManager::infoScreen() {
    display.clearBuffer();
    display.setCursor(0, 15);
    display.setTextSize(1);
    display.setTextWrap(true);
    display.setFont(&FreeSerif9pt7b);
    display.println("This device measures heart rate variability (HRV) and ");
    display.println("its reaction to symapthetic and parasympathetic stimuli.");
    display.println("The examination consists of four procedures:");
    display.println("1. Resting HRV measurement");
    display.println("2. Blood pressure measurement");
    display.println("3. Cuff constricted HRV measurement");
    display.println("4. Deep breathing HRV measurement");
    display.setFont(&FreeSerifBold9pt7b);
    display.println(" ");
    display.println("Press START to begin resting HRV measurement");
    display.println("Press STOP during any procedure to stop the exam");
    display.display();
}


void DisplayManager::baseHRVprog() {
    display.clearBuffer();
    display.setFont(&FreeSerif18pt7b);
    display.setTextSize(1);
    centerText("Measuring Resting HRV", 110);
    centerText(". . .", 130);
    display.setFont(&FreeSerif9pt7b);
    centerText("Press STOP at any time to stop", 150);
    display.display();
}

void DisplayManager::baseHRVresults(const float hr, const float rmssd) {
    display.clearBuffer();
    display.setCursor(0, 80);
    display.setTextSize(1);
    display.setFont(&FreeSerif12pt7b);
    display.print("Resting Heart Rate (BPM): ");
    display.println(hr);
    display.print("Resting HRV (RMSSD, ms): ");
    display.println(rmssd);
    display.setFont(&FreeSerifBold9pt7b);
    display.print("Press START to continue to blood pressure test");
    display.display();
}

void DisplayManager::BPprog() {
    display.clearBuffer();
    display.setFont(&FreeSerif18pt7b);
    centerText("Measuring Blood Pressure", 110);
    centerText(". . .", 130);
    display.setFont(&FreeSerif9pt7b);
    centerText("Press STOP at any time to stop", 150);
    display.display();
}

void DisplayManager::BPresults(const int SBP, const int DBP) {
    display.clearBuffer();
    display.setCursor(0, 80);
    display.setFont(&FreeSerif12pt7b);
    display.print("Blood Pressure (SYS/DIA): ");
    display.print(SBP);
    display.print("/");
    display.println(DBP);
    display.println(" ");
    display.setFont(&FreeSerifBold9pt7b);
    display.println("Press START to continue to HRV measurement");
    display.println("with cuff stimulus");
    display.display();
};

void DisplayManager::bpStimProg() {
    display.clearBuffer();
    display.setFont(&FreeSerif18pt7b);
    display.setTextSize(1);
    centerText("Measuring HRV", 110);
    display.setFont(&FreeSerif12pt7b);
    centerText("(With Cuff Stimulus)", 130);
    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press STOP at any time to stop", 150);
    display.display();
}

void DisplayManager::bpStimResults(const float hr, const float rmssd) {
    display.clearBuffer();
    display.setCursor(0, 80);
    display.setFont(&FreeSerif12pt7b);
    display.print("Cuff Stim. Heart Rate (BPM): ");
    display.println(hr);
    display.print("Cuff Stim. HRV (RMSSD, ms): ");
    display.println(rmssd);
    display.setFont(&FreeSerifBold9pt7b);
    display.println("Press START to continue to HRV measurement");
    display.println("with deep breathing stimulus");
    display.display();
}

void DisplayManager::spStimProg() {
    display.clearBuffer();
    display.setFont(&FreeSerif18pt7b);
    display.setTextSize(1);
    centerText("Measuring HRV", 110);
    display.setFont(&FreeSerif12pt7b);
    centerText("(With Breathing Stimulus)", 130);
    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press STOP at any time to stop", 150);
    display.display();
}

void DisplayManager::spStimResults(const float hr, const float rmssd, const float fvc) {
    display.clearBuffer();
    display.setCursor(0, 80);
    display.setFont(&FreeSerif12pt7b);
    display.print("Breath Stim. Heart Rate (BPM): ");
    display.println(hr);
    display.print("Breath Stim. HRV (RMSSD, ms): ");
    display.println(rmssd);
    display.print("Average Breath Volume (L): ");
    display.println(fvc);
    display.setFont(&FreeSerifBold9pt7b);
    display.println("Press START to view final results");
    display.display();
}

void DisplayManager::finalResults(const float rstHR, const float rstHRV, 
    const float SBP, const float DBP,
    const float bpHR, const float bpHRV,
    const float spHR, const float spHRV, const float fvc) {
    display.clearBuffer();
    display.setCursor(0, 15);
    display.setFont(&FreeSerif12pt7b);
    display.println("Full HRV Exam Results");
    display.setFont(&FreeSerif9pt7b);
    
    display.setCursor(0, 60);
    display.print("Resting HR, HRV: ");
    display.setFont(&FreeSerif12pt7b);
    display.print(rstHR);
    display.print(" BPM, ");
    display.print(rstHRV);
    display.println(" ms");

    display.setCursor(0, 90);
    display.setFont(&FreeSerif9pt7b);
    display.print("Cuff Stim. HR, HRV: ");
    display.setFont(&FreeSerif12pt7b);
    display.print(rstHR);
    display.print(" BPM, ");
    display.print(rstHRV);
    display.println(" ms");

    display.setCursor(0, 120);
    display.setFont(&FreeSerif9pt7b);
    display.print("Breath Stim. HR, HRV: ");
    display.setFont(&FreeSerif12pt7b);
    display.print(rstHR);
    display.print(" BPM, ");
    display.print(rstHRV);
    display.println(" ms");

    display.setCursor(0, 150);
    display.setFont(&FreeSerif9pt7b);
    display.print("Blood Pressure: ");
    display.setFont(&FreeSerif12pt7b);
    display.print(SBP);
    display.print("/");
    display.println(DBP);

    display.setCursor(0, 180);
    display.setFont(&FreeSerif9pt7b);
    display.print("Average Breath Volume: ");
    display.setFont(&FreeSerif12pt7b);
    display.print(fvc);
    display.println(" L");
    display.display();
    }