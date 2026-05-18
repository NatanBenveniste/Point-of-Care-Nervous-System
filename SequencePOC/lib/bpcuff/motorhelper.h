#include <Arduino.h>

//define pins
#define IN1 3
#define IN2 2
#define EEP 4
#define VALVE 10


class MotorControl {
public:
    MotorControl();
    
    void pumpForward();
    void stopPump();

    void valveOpen();
    void valveClose();
    void valvePulse(int ms);

    void setupHardware();

    void serialControl();
};
