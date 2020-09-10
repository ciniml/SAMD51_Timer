#include "SAMD51_Timer.hpp"

SAMD51TC TimerTC0(0);
SAMD51TC TimerTC1(1);
SAMD51TC TimerTC2(2);
SAMD51TC TimerTC3(3);
SAMD51TC TimerTC4(4);
SAMD51TC TimerTC5(5);
SAMD51TC TimerTC6(6);
SAMD51TC TimerTC7(7);

class SAMD51TCInterruptHelper
{
public:
    static void callProcessIsr(SAMD51TC& tc)
    {
        tc.processIsr();
    }
};

void TC0_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC0); }
void TC1_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC1); }
void TC2_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC2); }
void TC3_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC3); }
void TC4_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC4); }
void TC5_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC5); }
void TC6_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC6); }
void TC7_Handler() { SAMD51TCInterruptHelper::callProcessIsr(TimerTC7); }