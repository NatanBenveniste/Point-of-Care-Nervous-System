#include "heartrate.h"

HeartRateMonitor::HeartRateMonitor(){

}

void HeartRateMonitor::init() {
    pinMode(leadOffMinus, INPUT);
    pinMode(leadOffPlus, INPUT);
    pinMode(ecgPin, INPUT);
}

// void HeartRateMonitor::readECG() {
//     if((digitalRead(leadOffMinus) == 1)||(digitalRead(leadOffPlus) == 1)) {
//     Serial.println("Leads off");
//     }
//     else {
//         Serial.println(analogRead(ecgPin));      
//     }
// }

bool HeartRateMonitor::leadsOff() {
    return(digitalRead(leadOffMinus) == HIGH || digitalRead(leadOffPlus) == HIGH);
}

int HeartRateMonitor::readECG() {
    if (HeartRateMonitor::leadsOff()) {
        return -1;
    }

    return (analogRead(ecgPin));

};

