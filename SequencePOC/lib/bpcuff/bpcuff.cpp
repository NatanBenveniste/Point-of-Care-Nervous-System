#include "bpcuff.h"

CuffControl::CuffControl(){
}

//define functions
void CuffControl::pumpForward() {
  digitalWrite(EEP, HIGH);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void CuffControl::stopPump() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(EEP, LOW);
}

void CuffControl::valveOpen() {
  digitalWrite(VALVE, HIGH);
}

void CuffControl::valveClose() {
  digitalWrite(VALVE, LOW);
}

void CuffControl::valvePulse(int ms) {
  for (int i = 0; i < 3; i++) {
    valveOpen();
    delay(ms);
    valveClose();
    delay(ms);
  }
}

void CuffControl::setupHardware() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EEP, OUTPUT);
  pinMode(VALVE, OUTPUT);

  delay(100);

  Serial.println("Hardware setup");
}

void CuffControl::serialControl() {
  if (!Serial.available()) return;


  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  
  if (cmd == "pump") {
    CuffControl::pumpForward();
    Serial.println("ACK,pump");
  }

  else if (cmd == "stop") {
    CuffControl::stopPump();
    CuffControl::valveClose();
    Serial.println("ACK,stop");
  }

  else if (cmd == "valve") {
    CuffControl::valveOpen();
    Serial.println("ACK,valve");
  }

  else if (cmd == "close") {
    CuffControl::valveClose();
    Serial.println("ACK,close");
  }

  else if (cmd == "pulse") {
    CuffControl::valvePulse(100);
    Serial.println("ACK,pulse");
  }
    else if (cmd == "bp") {
    CuffControl::runBPMeasurement();
    Serial.println("ACK,bp");
  }

  else if (cmd == "hold5") {
    CuffControl::runFiveMinuteHold();
    Serial.println("ACK,hold5");
  }

  else if (cmd == "deflate") {
    CuffControl::fullDeflate();
    Serial.println("ACK,deflate");
  }
}
//sequence 1: BP cuff measurement
void CuffControl::runBPMeasurement() {
  Serial.println("BP,start");

  //close valve so cuff can inflate
  CuffControl::valveClose();

  //start pump
  CuffControl::pumpForward();
  Serial.println("BP,pumping");

  //inflate for set time
  //change this number after testing
  delay(15000);

  //stop pump
  CuffControl::stopPump();
  Serial.println("BP,pump_stop");

  //small wait before deflating
  delay(1000);

  //open valve to slowly deflate
  CuffControl::valveOpen();
  Serial.println("BP,deflating");

  //this is where measurement/recording would happen
  //change this number based on how long deflation takes
  delay(30000);

  //close valve after measurement
  CuffControl::valveClose();

  Serial.println("BP,done");
}


//sequence 2: inflate cuff, hold 5 minutes, then deflate
void CuffControl::runFiveMinuteHold() {
  Serial.println("HOLD,start");

  //close valve so cuff can inflate
  CuffControl::valveClose();

  //start pump
  CuffControl::pumpForward();
  Serial.println("HOLD,pumping");

  //inflate for set time
  //change this number after testing
  delay(15000);

  //stop pump
  CuffControl::stopPump();
  Serial.println("HOLD,pump_stop");

  //keep valve closed and hold pressure
  CuffControl::valveClose();
  Serial.println("HOLD,holding_5_minutes");

  //hold for 5 minutes
  delay(300000);

  //deflate cuff
  Serial.println("HOLD,deflating");
  CuffControl::valveOpen();

  //leave valve open long enough to fully deflate
  delay(30000);

  //close valve after deflation
  CuffControl::valveClose();

  Serial.println("HOLD,done");
}


//manual full deflate sequence
void CuffControl::fullDeflate() {
  Serial.println("DEFLATE,start");

  CuffControl::stopPump();
  CuffControl::valveOpen();

  delay(30000);

  CuffControl::valveClose();

  Serial.println("DEFLATE,done");
}

