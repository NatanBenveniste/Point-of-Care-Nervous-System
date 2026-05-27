#include <Arduino.h>
#include <SPI.h>
#include <motorhelper.h>
#include <displaymanager.h>


#define chargePin 26
bool showCharge = false;

#define buttonPin 27


MotorControl control;
DisplayManager screen;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("serial");
  pinMode(EPD_BUSY, INPUT_PULLUP);

  SPI1.setSCK(10);
  SPI1.setTX(11);
  SPI1.setRX(12);
  SPI1.begin();
  Serial.println("spi check");
  Serial.println(SPI1.setSCK(10));
  Serial.println(SPI1.setTX(11));
  Serial.println(SPI1.setRX(12));

  Serial.print("BUSY before init: ");
  Serial.println(digitalRead(EPD_BUSY));

  screen.init();

  Serial.print("BUSY after init: ");
  Serial.println(digitalRead(EPD_BUSY));

  // screen.fillScreenTest('b');
  screen.runDemo();

  Serial.print("BUSY after draw: ");
  Serial.println(digitalRead(EPD_BUSY));
}

void loop() {  
  Serial.print("BUSY idle: ");
  Serial.println(digitalRead(EPD_BUSY));
  delay(500);

}