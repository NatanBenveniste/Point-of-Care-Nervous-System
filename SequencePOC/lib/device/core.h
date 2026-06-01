#include <Arduino.h>
#include <SPI.h>

#define chargePin 26
#define startButtonPin 27
#define stopButtonPin 20


void coreInit();

bool startPressed();
bool stopPressed();

void waitStart();