#include "webmanager.h"

const char* AP_SSID = "67-Device";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebManager::WebManager()
    : server(80)
{
    connectedLive = false;

    pageTitle = "67 HRV MONITOR";
    pageMessage = "Press START to begin exam";
    procedureStatus = "Waiting to begin";
}

void WebManager::begin() {
    Serial.println("Starting Pico 2 W open access point...");

    WiFi.mode(WIFI_AP);

    bool configOK = WiFi.softAPConfig(local_IP, gateway, subnet);

    if (configOK) {
        Serial.println("AP IP config set.");
    } else {
        Serial.println("AP IP config failed.");
    }

    bool apOK = WiFi.softAP(AP_SSID);

    if (apOK) {
        Serial.println("Access point started.");
        connectedLive = true;
    } else {
        Serial.println("Access point FAILED to start.");
        connectedLive = false;
    }

    delay(1000);

    Serial.println();
    Serial.print("WiFi network name: ");
    Serial.println(AP_SSID);
    Serial.println("Password: none");
    Serial.print("Open this address in your browser: ");
    Serial.println("http://192.168.4.1");

    server.on("/", [this]() {
        handleRoot();
    });

    server.begin();

    Serial.println("Web server started.");
}

void WebManager::tick() {
    server.handleClient();

    // The Pico is hosting the access point and web server.
    // This means the web page is available from the device.
    connectedLive = true;
}

void WebManager::handleRoot() {
    server.send(200, "text/html", makeHtmlPage());
}

void WebManager::startScreen() {
    pageTitle = "67 HRV MONITOR";
    pageMessage = "Press START to begin exam";
    procedureStatus = "Waiting to begin";
}

void WebManager::infoScreen() {
    pageTitle = "Exam Information";

    pageMessage =
        "<div class='left readable'>"
        "<p>This device measures heart rate variability (HRV) and its reaction to sympathetic and parasympathetic stimuli.</p>"
        "<p>The examination consists of four procedures:</p>"
        "<ol>"
        "<li>Resting HRV measurement</li>"
        "<li>Blood pressure measurement</li>"
        "<li>Cuff constricted HRV measurement</li>"
        "<li>Deep breathing HRV measurement</li>"
        "</ol>"
        "<p class='prompt'>Press START to begin resting HRV measurement.</p>"
        "<p class='smallPrompt'>Press STOP during any procedure to stop the exam.</p>"
        "</div>";

    procedureStatus = "Information screen";
}

void WebManager::baseHRVprog() {
    pageTitle = "Measuring Resting HRV";

    pageMessage =
        "<div class='center readable'>"
        "<h1>Measuring Resting HRV</h1>"
        "<div class='ellipsis'>. . .</div>"
        "<p>Press STOP at any time to stop.</p>"
        "</div>";

    procedureStatus = "Measuring resting HRV";
}

void WebManager::baseHRVresults(const float hr, const float rmssd) {
    pageTitle = "Resting HRV Results";

    pageMessage =
        "<div class='resultBlock'>"
        "<p><strong>Resting Heart Rate (BPM):</strong> " + String(hr, 1) + "</p>"
        "<p><strong>Resting HRV (RMSSD, ms):</strong> " + String(rmssd, 1) + "</p>"
        "<p class='prompt'>Press START to continue to blood pressure test.</p>"
        "</div>";

    procedureStatus = "Resting HRV complete";
}

void WebManager::BPprog() {
    pageTitle = "Measuring Blood Pressure";

    pageMessage =
        "<div class='center readable'>"
        "<h1>Measuring Blood Pressure</h1>"
        "<div class='ellipsis'>. . .</div>"
        "<p>Press STOP at any time to stop.</p>"
        "</div>";

    procedureStatus = "Measuring blood pressure";
}

void WebManager::BPresults(const int SBP, const int DBP) {
    pageTitle = "Blood Pressure Results";

    pageMessage =
        "<div class='resultBlock'>"
        "<p><strong>Blood Pressure (SYS/DIA):</strong> " + String(SBP) + "/" + String(DBP) + "</p>"
        "<p class='prompt'>Press START to continue to HRV measurement with cuff stimulus.</p>"
        "</div>";

    procedureStatus = "Blood pressure complete";
}

void WebManager::bpStimProg() {
    pageTitle = "Measuring HRV";

    pageMessage =
        "<div class='center readable'>"
        "<h1>Measuring HRV</h1>"
        "<p class='subhead'>(With Cuff Stimulus)</p>"
        "<p class='prompt'>Press STOP at any time to stop.</p>"
        "</div>";

    procedureStatus = "Measuring HRV with cuff stimulus";
}

void WebManager::bpStimResults(const float hr, const float rmssd) {
    pageTitle = "Cuff Stimulus HRV Results";

    pageMessage =
        "<div class='resultBlock'>"
        "<p><strong>Cuff Stim. Heart Rate (BPM):</strong> " + String(hr, 1) + "</p>"
        "<p><strong>Cuff Stim. HRV (RMSSD, ms):</strong> " + String(rmssd, 1) + "</p>"
        "<p class='prompt'>Press START to continue to HRV measurement with deep breathing stimulus.</p>"
        "</div>";

    procedureStatus = "Cuff stimulus HRV complete";
}

void WebManager::spStimProg() {
    pageTitle = "Measuring HRV";

    pageMessage =
        "<div class='center readable'>"
        "<h1>Measuring HRV</h1>"
        "<p class='subhead'>(With Breathing Stimulus)</p>"
        "<p class='prompt'>Press STOP at any time to stop.</p>"
        "</div>";

    procedureStatus = "Measuring HRV with breathing stimulus";
}

void WebManager::spStimResults(const float hr, const float rmssd, const float fvc) {
    pageTitle = "Breathing Stimulus HRV Results";

    pageMessage =
        "<div class='resultBlock'>"
        "<p><strong>Breath Stim. Heart Rate (BPM):</strong> " + String(hr, 1) + "</p>"
        "<p><strong>Breath Stim. HRV (RMSSD, ms):</strong> " + String(rmssd, 1) + "</p>"
        "<p><strong>Average Breath Volume (L):</strong> " + String(fvc, 2) + "</p>"
        "<p class='prompt'>Press START to view final results.</p>"
        "</div>";

    procedureStatus = "Breathing stimulus HRV complete";
}

void WebManager::finalResults(
    const float rstHR, const float rstHRV,
    const int SBP, const int DBP,
    const float bpHR, const float bpHRV,
    const float spHR, const float spHRV,
    const float fvc
) {
    pageTitle = "Full HRV Exam Results";

    pageMessage =
        "<div class='resultBlock'>"
        "<h1>Full HRV Exam Results</h1>"
        "<p><strong>Resting HR, HRV:</strong> " + String(rstHR, 1) + " BPM, " + String(rstHRV, 1) + " ms</p>"
        "<p><strong>Cuff Stim. HR, HRV:</strong> " + String(bpHR, 1) + " BPM, " + String(bpHRV, 1) + " ms</p>"
        "<p><strong>Breath Stim. HR, HRV:</strong> " + String(spHR, 1) + " BPM, " + String(spHRV, 1) + " ms</p>"
        "<p><strong>Blood Pressure:</strong> " + String(SBP) + "/" + String(DBP) + "</p>"
        "<p><strong>Average Breath Volume:</strong> " + String(fvc, 2) + " L</p>"
        "</div>";

    procedureStatus = "Exam complete";
}

String WebManager::makeHtmlPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="refresh" content="1">
  <title>67 HRV Monitor</title>

  <style>
    :root {
      --bg: #101418;
      --card: #f7f3ea;
      --text: #151515;
      --muted: #5f5f5f;
      --good: #22c55e;
      --bad: #ef4444;
    }

    body {
      margin: 0;
      padding: 0;
      min-height: 100vh;
      background: radial-gradient(circle at top, #263241 0%, #101418 65%);
      font-family: Georgia, "Times New Roman", serif;
      color: var(--text);
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .phone {
      width: min(92vw, 760px);
      min-height: 420px;
      background: var(--card);
      border-radius: 28px;
      box-shadow: 0 22px 60px rgba(0, 0, 0, 0.35);
      overflow: hidden;
      border: 1px solid rgba(255, 255, 255, 0.25);
    }

    .topbar {
      background: #151515;
      color: white;
      padding: 14px 20px;
      font-family: Arial, sans-serif;
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 16px;
      font-size: 14px;
    }

    .brand {
      font-weight: 700;
      letter-spacing: 0.08em;
      text-transform: uppercase;
    }

    .connection {
      display: flex;
      align-items: center;
      gap: 8px;
      white-space: nowrap;
    }

    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--bad);
      box-shadow: 0 0 10px rgba(239, 68, 68, 0.85);
    }

    .dot.connected {
      background: var(--good);
      box-shadow: 0 0 10px rgba(34, 197, 94, 0.85);
    }

    .content {
      min-height: 330px;
      padding: 42px 44px;
      box-sizing: border-box;
      display: flex;
      flex-direction: column;
      justify-content: center;
      text-align: center;
    }

    .title {
      font-size: clamp(34px, 7vw, 58px);
      font-weight: 700;
      letter-spacing: 0.03em;
      margin: 0 0 18px;
    }

    .center {
      text-align: center;
      margin: 0 auto;
    }

    .left {
      text-align: left;
      margin: 0 auto;
    }

    .readable {
      width: min(100%, 650px);
      font-size: clamp(17px, 2.8vw, 23px);
      line-height: 1.5;
      overflow-wrap: break-word;
      word-wrap: break-word;
    }

    .readable h1,
    .resultBlock h1 {
      font-size: clamp(30px, 5vw, 46px);
      line-height: 1.15;
      margin: 0 0 18px;
      text-align: center;
    }

    .readable p,
    .resultBlock p {
      margin: 10px 0;
    }

    .readable ol {
      margin: 12px 0 18px 1.5em;
      padding: 0;
    }

    .readable li {
      margin: 8px 0;
    }

    .headline {
      font-size: clamp(28px, 5vw, 44px);
      line-height: 1.15;
      margin: 0 0 10px;
    }

    .subhead {
      font-size: clamp(20px, 3.8vw, 30px);
      line-height: 1.25;
      margin: 8px 0 18px;
      font-weight: 700;
    }

    .body {
      font-size: clamp(16px, 2.8vw, 22px);
      line-height: 1.42;
      margin: 6px 0;
    }

    .bold {
      font-weight: 700;
    }

    .resultBlock {
      width: min(100%, 680px);
      text-align: left;
      margin: 0 auto;
      font-size: clamp(17px, 2.8vw, 24px);
      line-height: 1.45;
      overflow-wrap: break-word;
      word-wrap: break-word;
    }

    .resultLine {
      margin: 10px 0;
    }

    .prompt {
      margin-top: 24px !important;
      font-weight: 700;
      font-size: clamp(17px, 2.7vw, 22px);
      line-height: 1.35;
    }

    .smallPrompt {
      font-weight: 700;
      font-size: clamp(15px, 2.4vw, 19px);
      line-height: 1.35;
    }

    .ellipsis {
      font-size: clamp(34px, 6vw, 52px);
      letter-spacing: 0.18em;
      margin: 2px 0 14px;
    }

    .footer {
      background: rgba(0, 0, 0, 0.04);
      color: var(--muted);
      padding: 12px 20px;
      font-family: Arial, sans-serif;
      font-size: 13px;
      text-align: center;
    }

    @media (max-width: 520px) {
      .content {
        padding: 34px 24px;
      }

      .topbar {
        align-items: flex-start;
        flex-direction: column;
        gap: 8px;
      }
    }
  </style>
</head>

<body>
  <main class="phone">
    <div class="topbar">
      <div class="brand">67 HRV Monitor</div>
      <div class="connection">
)rawliteral";

    if (connectedLive) {
        html += "        <span class='dot connected'></span>\n";
        html += "        <span>Device connected live</span>\n";
    } else {
        html += "        <span class='dot'></span>\n";
        html += "        <span>Device disconnected</span>\n";
    }

    html += R"rawliteral(
      </div>
    </div>

    <section class="content">
)rawliteral";

    if (pageTitle == "67 HRV MONITOR") {
        html += "      <h1 class='title'>" + pageTitle + "</h1>\n";
        html += "      <div class='subhead'>" + pageMessage + "</div>\n";
    } else {
        html += pageMessage;
    }

    html += R"rawliteral(
    </section>

    <div class="footer">
)rawliteral";

    html += "      " + procedureStatus + "\n";

    html += R"rawliteral(
    </div>
  </main>
</body>
</html>
)rawliteral";

    return html;
}