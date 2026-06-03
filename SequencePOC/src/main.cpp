#include <Arduino.h>
#include "webmanager.h"

WebManager web;

float rstHR = 72.4;
float rstHRV = 41.8;

int SBP = 118;
int DBP = 76;

float bpHR = 84.2;
float bpHRV = 28.6;

float spHR = 69.7;
float spHRV = 52.3;
float fvc = 3.42;

void serviceSystem() {
    web.tick();
}

void fakeProcedureDelay(unsigned long durationMs) {
    unsigned long startTime = millis();

    while (millis() - startTime < durationMs) {
        serviceSystem();
        delay(5);
    }
}

void waitForSerialNext() {
    Serial.println("Type n to continue.");

    while (true) {
        serviceSystem();

        if (Serial.available()) {
            char c = Serial.read();

            if (c == 'n' || c == 'N') {
                break;
            }
        }

        delay(5);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.println("Starting fixed-sequence WebManager test...");

    web.begin();

    Serial.println("Connect to WiFi network: 67-Device");
    Serial.println("Open browser to: http://192.168.4.1");

    web.startScreen();
    waitForSerialNext();

    web.infoScreen();
    waitForSerialNext();

    web.baseHRVprog();
    fakeProcedureDelay(5000);
    web.baseHRVresults(rstHR, rstHRV);
    waitForSerialNext();

    web.BPprog();
    fakeProcedureDelay(5000);
    web.BPresults(SBP, DBP);
    waitForSerialNext();

    web.bpStimProg();
    fakeProcedureDelay(5000);
    web.bpStimResults(bpHR, bpHRV);
    waitForSerialNext();

    web.spStimProg();
    fakeProcedureDelay(5000);
    web.spStimResults(spHR, spHRV, fvc);
    waitForSerialNext();

    web.finalResults(
        rstHR, rstHRV,
        SBP, DBP,
        bpHR, bpHRV,
        spHR, spHRV,
        fvc
    );

    Serial.println("Test sequence complete.");
}

void loop() {
    web.tick();
}