#include "core.h"

void coreInit() {
    pinMode(chargePin, INPUT);
    pinMode(startButtonPin, INPUT_PULLUP);
    pinMode(stopButtonPin, INPUT_PULLUP);

    SPI1.setSCK(10);
    SPI1.setTX(11);
    SPI1.setRX(12);
    SPI1.begin();
}

bool startPressed() {
  static bool lastState = HIGH;
  static uint32_t lastChange = 0;

  bool currentState = digitalRead(startButtonPin);

  if (currentState != lastState) {
    lastChange = millis();
    lastState = currentState;
  }

  if ((millis() - lastChange) > 30 && currentState == LOW) {
    while (digitalRead(startButtonPin) == LOW) {
      delay(1);
    }
    return true;
  }

  return false;
}

bool abortPressed() {
  static bool lastState = HIGH;
  static uint32_t lastChange = 0;

  bool currentState = digitalRead(stopButtonPin);

  if (currentState != lastState) {
    lastChange = millis();
    lastState = currentState;
  }

  if ((millis() - lastChange) > 30 && currentState == LOW) {
    while (digitalRead(stopButtonPin) == LOW) {
      delay(1);
    }
    return true;
  }

  return false;
}

void waitStart() {
    while(true) {
        if (startPressed()) {
            return;
        }
    }
}