#include <Arduino.h>

void setup() {
// initialize the serial communication:
Serial.begin(115200);
pinMode(21, INPUT); // Setup for leads off detection LO +
pinMode(22, INPUT); // Setup for leads off detection LO -
 
}
 
void loop() {
 
if((digitalRead(21) == 1)||(digitalRead(22) == 1)){
Serial.println('!');
}
else{
// send the value of analog input 0:
Serial.println(analogRead(28));
}
//Wait for a bit to keep serial data from saturating
delay(4);
}