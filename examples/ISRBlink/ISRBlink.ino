#include "SAMD51_Timer.hpp"

bool isLEDOn = false;
char time = 0;

void setup() 
{
    pinMode(LED_BUILTIN, OUTPUT);    

    //TimerTC3.initialize(1000000000ul);
    TimerTC3.initialize(1000000000ul, GCLK_XOSC32K);
    TimerTC3.attachInterrupt(timerIsr);
}
 
void loop()
{
}

void timerIsr(void* context)
{    
    digitalWrite(LED_BUILTIN, isLEDOn);
    isLEDOn = !isLEDOn;
}