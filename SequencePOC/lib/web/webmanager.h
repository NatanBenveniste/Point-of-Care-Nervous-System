#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebManager {
public:
    WebManager();

    void begin();
    void tick();

    void startScreen();
    void infoScreen();

    void baseHRVprog();
    void baseHRVresults(const float hr, const float rmssd);

    void BPprog();
    void BPresults(const int SBP, const int DBP);

    void bpStimProg();
    void bpStimResults(const float hr, const float rmssd);

    void spStimProg();
    void spStimResults(const float hr, const float rmssd, const float fvc);

    void finalResults(
        const float rstHR, const float rstHRV,
        const int SBP, const int DBP,
        const float bpHR, const float bpHRV,
        const float spHR, const float spHRV,
        const float fvc
    );

private:
    WebServer server;

    bool connectedLive;

    String pageTitle;
    String pageMessage;
    String procedureStatus;

    void handleRoot();
    String makeHtmlPage();
};