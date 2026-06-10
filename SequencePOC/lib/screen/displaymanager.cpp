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

    Serial.print("Busy pin state: "); 
    Serial.println(digitalRead(EPD_BUSY));

    // display.begin(THINKINK_TRICOLOR);
    display.begin(THINKINK_MONO);
    display.setRotation(2); // 0 for flat, 2 for in device
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

void DisplayManager::centerText(const char *text, const int y) {
    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

    int16_t x = (display.width() - w) / 2;
    if (x < 0) {
        x = 0;
    }

    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::drawHeader(const String &title) {
    display.setFont(&FreeSerif12pt7b);
    display.setTextSize(1);
    centerText(title.c_str(), 32);

    display.drawLine(10, 42, display.width() - 10, 42, EPD_BLACK);
}

void DisplayManager::drawPrompt(const String &text, const int y) {
    display.setFont(&FreeSerifBold9pt7b);
    display.setTextSize(1);
    centerText(text.c_str(), y);
}

void DisplayManager::drawBatteryVoltage(const float vBat) {
    String voltageText = String(vBat, 2) + "V";
    
    display.setFont(&FreeSerif9pt7b);
    display.setTextSize(1);
    display.setTextColor(EPD_BLACK);

    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(voltageText.c_str(), 0, 16, &x1, &y1, &w, &h);

    int16_t x = display.width() - w - 6;
    if (x < 0) {
        x = 0;
    }

    display.setCursor(x, 15);
    display.print(voltageText);
}

void DisplayManager::printLine(const String &text, const int x, const int y) {
    display.setFont(&FreeSerif9pt7b);
    display.setTextSize(1);
    display.setCursor(x, y);
    display.println(text);
}

void DisplayManager::printValueLine(const String &label, const String &value, const int x, const int y) {
    display.setFont(&FreeSerifBold9pt7b);
    display.setTextSize(1);
    display.setCursor(x, y);
    display.print(label);

    display.setFont(&FreeSerif9pt7b);
    display.print(value);
}

// menu functions
void DisplayManager::startScreen(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerifBold9pt7b);
    display.setTextSize(2);
    centerText("67 HRV", 125);
    centerText("MONITOR", 158);

    display.setTextSize(1);
    display.setFont(&FreeSerif12pt7b);
    centerText("Press START to begin exam", 215);

    display.setFont(&FreeSerif9pt7b);
    centerText("Device working voltage: 3.0-4.2V", 340);

    display.display();
}

void DisplayManager::infoScreen(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    drawHeader("Exam Information");

    display.setTextSize(1);
    display.setFont(&FreeSerif9pt7b);
    display.setCursor(0, 60);
    display.println("  This device measures heart rate");
    display.println("  variability (HRV) and its response");
    display.println("  to sympathetic and parasympathetic");
    display.println("  stimuli using a blood pressure cuff");
    display.println("  and a spirometer.");

    display.setFont(&FreeSerifBold9pt7b);
    display.println("The exam consists of four procedures:");
    display.setFont(&FreeSerif9pt7b);
    display.println("  1. Resting HRV measurement");
    display.println("  2. Blood pressure measurement");
    display.println("  3. Cuff constricted HRV measurement");
    display.println("  4. Deep breathing HRV measurement");
    display.println("");

    display.println("  Remain seated and avoid movements");
    display.println("  that may distort the results.");

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press START to begin resting", 345);
    centerText("HRV measurement", 370);

    display.display();
}

void DisplayManager::baseHRVprog(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerif18pt7b);
    centerText("Measuring", 100);
    centerText("Resting HRV", 138);

    display.setFont(&FreeSerif24pt7b);
    centerText(". . .", 175);

    display.setFont(&FreeSerif9pt7b);
    centerText("Please remain seated, relaxed, and still", 215);
    centerText("while resting HRV is measured.", 240);
    centerText("Avoid moving the chest strap", 265);
    centerText("or changing posture.", 290);

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press STOP at any time to stop", 340);

    display.display();
}

void DisplayManager::baseHRVresults(const float hr, const float rmssd, const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    drawHeader("Resting HRV Results");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 110);
    display.print("Resting Heart Rate:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 140);
    display.print(String(hr, 1));
    display.println(" BPM");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 200);
    display.print("Resting HRV:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 230);
    display.print(String(rmssd, 1));
    display.println(" ms");

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press START to continue to", 340);
    centerText("blood pressure test", 365);

    display.display();
}

void DisplayManager::BPprog(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerif18pt7b);
    centerText("Measuring", 100);
    centerText("Blood Pressure", 138);

    display.setFont(&FreeSerif24pt7b);
    centerText(". . .", 175);

    display.setFont(&FreeSerif9pt7b);
    centerText("Please keep your arm still and relaxed", 215);
    centerText("while the cuff inflates and deflates.", 240);
    centerText("Remain seated and avoid moving", 265);
    centerText("during the measurement.", 290);

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press STOP at any time to stop", 340);

    display.display();
}

void DisplayManager::BPresults(const int SBP, const int DBP, const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    drawHeader("Blood Pressure Results");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 110);
    display.print("Blood Pressure:");

    display.setFont(&FreeSerif12pt7b);
    display.setCursor(35, 140);
    display.print(String(SBP));
    display.print("/");
    display.println(String(DBP));

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press START to continue to", 315);
    centerText("HRV measurement with", 340);
    centerText("cuff stimulus", 365);

    display.display();
}

void DisplayManager::bpStimProg(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerif18pt7b);
    centerText("Measuring HRV", 100);

    display.setFont(&FreeSerif9pt7b);
    centerText("(With 5 Minute Cuff Stimulus)", 128);

    display.setFont(&FreeSerif24pt7b);
    centerText(". . .", 170);

    display.setFont(&FreeSerif9pt7b);
    centerText("Press START again after cuff is fully", 215);
    centerText("inflated to begin the holding", 240);
    centerText("measurement.", 265);
    centerText("Please remain seated, still, and ", 290);
    centerText("keep your arm relaxed.", 315);

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press STOP at any time to stop", 340);

    display.display();
}

void DisplayManager::bpStimResults(const float hr, const float rmssd, const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    drawHeader("Cuff Stim. HRV Results");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 110);
    display.print("Cuff Stim. Heart Rate:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 140);
    display.print(String(hr, 1));
    display.println(" BPM");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 200);
    display.print("Cuff Stim. HRV:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 230);
    display.print(String(rmssd, 1));
    display.println(" ms");

    centerText("Reorient the device to access", 265);
    centerText("the spirometer mouthpiece.", 280);

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press START to deflate cuff.", 315);
    centerText("Press START again to begin HRV", 340);
    centerText("measurement w/ breathing stimulus.", 365);

    display.display();
}

void DisplayManager::spStimProg(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerif18pt7b);
    centerText("Measuring HRV", 100);

    display.setFont(&FreeSerif9pt7b);
    centerText("(With 1 Minute Breathing Stimulus)", 128);

    display.setFont(&FreeSerif24pt7b);
    centerText(". . .", 170);

    display.setFont(&FreeSerif9pt7b);
    centerText("Please remain seated and take deep,", 215);
    centerText("even breaths into the spirometer.", 240);
    centerText("Do not inhale back through", 265);
    centerText("the spirometer.", 290);

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press STOP at any time to stop", 340);

    display.display();
}

void DisplayManager::spStimResults(
    const float hr,
    const float rmssd,
    const float fvc,
    const float vBat
) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    drawHeader("Breathing Stim. HRV Results");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 110);
    display.print("Breath Stim. Heart Rate:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 140);
    display.print(String(hr, 1));
    display.println(" BPM");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 190);
    display.print("Breath Stim. HRV:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 220);
    display.print(String(rmssd, 1));
    display.println(" ms");

    display.setFont(&FreeSerifBold9pt7b);
    display.setCursor(16, 270);
    display.print("Average Breath Volume:");

    display.setFont(&FreeSerif9pt7b);
    display.setCursor(28, 300);
    display.print(String(fvc, 2));
    display.println(" L");

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Press START to view final results", 350);
    display.display();
}

void DisplayManager::finalResults(
    const float rstHR, const float rstHRV,
    const int SBP, const int DBP,
    const float bpHR, const float bpHRV,
    const float spHR, const float spHRV,
    const float fvc,
    const float vBat
) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    drawHeader("Full HRV Exam Results");

    display.setCursor(10, 70);
    display.setFont(&FreeSerif9pt7b);
    display.println("Resting HR, HRV:");
    display.setCursor(24, 95);
    display.setFont(&FreeSerifBold9pt7b);
    display.print(String(rstHR, 1));
    display.print(" BPM, ");
    display.print(String(rstHRV, 1));
    display.println(" ms");

    
    display.setCursor(10, 125);
    display.setFont(&FreeSerif9pt7b);
    display.println("Cuff Stimulus HR, HRV:");
    display.setCursor(24, 150);
    display.setFont(&FreeSerifBold9pt7b);
    display.print(String(bpHR, 1));
    display.print(" BPM, ");
    display.print(String(bpHRV, 1));
    display.println(" ms");

    display.setCursor(10, 180);
    display.setFont(&FreeSerif9pt7b);
    display.println("Breath Stimulus HR, HRV:");
    display.setCursor(24, 205);
    display.setFont(&FreeSerifBold9pt7b);
    display.print(String(spHR, 1));
    display.print(" BPM, ");
    display.print(String(spHRV, 1));
    display.println(" ms");

    display.setCursor(10, 235);
    display.setFont(&FreeSerif9pt7b);
    display.print("Blood Pressure: ");
    display.setFont(&FreeSerifBold9pt7b);
    display.print(String(SBP));
    display.print("/");
    display.println(String(DBP));

    display.setCursor(10, 265);
    display.setFont(&FreeSerif9pt7b);
    display.print("Average Breath Volume: ");
    display.setFont(&FreeSerifBold9pt7b);
    display.print(String(fvc, 2));
    display.println(" L");

    display.setFont(&FreeSerifBold9pt7b);
    centerText("Exam complete.", 320);
    centerText("Press STOP then START", 340);
    centerText("to rerun exam.", 360);

    display.display();
}

void DisplayManager::stopScreen(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerif18pt7b);
    centerText("Stop Button", 120);
    centerText("Pressed", 150);

    display.setFont(&FreeSerif24pt7b);
    centerText(". . .", 185);

    display.setFont(&FreeSerif9pt7b);
    centerText("HRV exam has been aborted.", 240);
    centerText("Press START to restart exam", 300);
    centerText("from base HRV.", 325);

    display.display();
}

void DisplayManager::errorScreen(const float vBat) {
    display.clearBuffer();
    drawBatteryVoltage(vBat);

    display.setFont(&FreeSerif18pt7b);
    centerText("ERROR", 140);

    display.setFont(&FreeSerif24pt7b);
    centerText(". . .", 185);

    display.setFont(&FreeSerif9pt7b);
    centerText("Error detected during HRV exam.", 240);
    centerText("Press START to restart exam", 300);
    centerText("from base HRV.", 325);

    display.display();
}