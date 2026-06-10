#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebManager {
public:
    WebManager();

    void begin();
    void tick();

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
        const float spHR, const float spHRV, const float fvc,
        const float vBat
    );

    void stopScreen(const float vBat);
    void errorScreen(const float vBat);

private:
    WebServer server;

    bool connectedLive;

    String pageTitle;
    String pageMessage;
    String procedureStatus;

    void handleRoot();
    String makeHtmlPage();
    String batteryHtml(const float vBat);
};