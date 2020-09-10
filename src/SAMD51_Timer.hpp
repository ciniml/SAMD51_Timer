#ifndef SAMD51_TIMER_HPP__
#define SAMD51_TIMER_HPP__

#include <samd.h>

#include <cstdint>
#include <array>

#define GENERIC_CLOCK_GENERATOR_1M        (5u)
#define GENERIC_CLOCK_GENERATOR_XOSC32K   (3u)

class SAMD51TCInterruptHelper;

/**
 * @brief Struct which describes a GCLK.
 */
struct GCLKDescription {
    /**
     * @brief ID of GCLK
     */
    std::uint8_t id;
    /**
     * @brief Frequency of GCLK output clock in Hz.
     */
    std::uint32_t frequency;

    constexpr GCLKDescription(std::uint8_t id, std::uint32_t frequency) : id(id), frequency(frequency) {}
};

/**
 * @brief GCLK description for GCLK whose output clock is 1[MHz].
 */
static constexpr const GCLKDescription GCLK_1M(GENERIC_CLOCK_GENERATOR_1M, 1000000);
/**
 * @brief GCLK description for GCLK whose output clock is 32.768[kHz] and generated from XOSC32K.
 */
static constexpr const GCLKDescription GCLK_XOSC32K(GENERIC_CLOCK_GENERATOR_XOSC32K, 32768);

/**
 * @brief SAMD51 TC peripheral dri1Mver.
 */
class SAMD51TC
{
public:
    /**
     * @brief Create a periodic  new instance of SAM51TC for the TCx peripheral where x == tcUnit.
     * @param [in] tcUnit index of TC peripheral.
     */
    SAMD51TC(std::uint_least8_t tcUnit) : tcUnit(tcUnit), regs(nullptr), isrCallback(nullptr), nanoseconds(0)
    {
        switch(tcUnit)
        {
            case 0: {this->regs = &TC0->COUNT16; this->irqn = TC0_IRQn; break;}
            case 1: {this->regs = &TC1->COUNT16; this->irqn = TC1_IRQn; break;}
            case 2: {this->regs = &TC2->COUNT16; this->irqn = TC2_IRQn; break;}
            case 3: {this->regs = &TC3->COUNT16; this->irqn = TC3_IRQn; break;}
            case 4: {this->regs = &TC4->COUNT16; this->irqn = TC4_IRQn; break;}
            case 5: {this->regs = &TC5->COUNT16; this->irqn = TC5_IRQn; break;}
            case 6: {this->regs = &TC6->COUNT16; this->irqn = TC6_IRQn; break;}
            case 7: {this->regs = &TC7->COUNT16; this->irqn = TC7_IRQn; break;}
        }
    }

    /**
     * @brief Initialize TC as a periodic timer.
     * @param [in] nanoseconds The period of the periodic timer in nanoseconds.
     */
    void initialize(std::uint32_t nanoseconds = 1000000, GCLKDescription gclk = GCLK_1M)
    {
        this->gclk = gclk;
        this->configureClock(gclk.id);

        this->regs->CTRLA.bit.SWRST = 1;
        while(this->regs->SYNCBUSY.bit.SWRST);

        this->setPeriod(nanoseconds);
    }

    /**
     * @brief Set period of this periodic timer.
     * @param [in] nanoseconds The period of this periodic timer.
     */ 
    void setPeriod(std::uint64_t nanoseconds)
    {
        this->nanoseconds = nanoseconds;
    }

    /**
     * @brief Starts timer.
     */
    void start()
    {
        this->regs->CTRLA.bit.ENABLE = 0;
        while(this->regs->SYNCBUSY.bit.ENABLE);

        this->regs->COUNT.reg = 0;
        while(this->regs->SYNCBUSY.bit.COUNT);

        const auto base_compare_value = (this->nanoseconds * this->gclk.frequency / 1000000000ul);
        const std::array<uint_fast8_t, 8> prescaler_shifts = { 0, 1, 2, 3, 4, 6, 8, 10 };
        for( std::uint_fast8_t index = 0; index < prescaler_shifts.size(); index++ ) {
            const auto compare_value = base_compare_value >> prescaler_shifts[index];
            if( compare_value <= 65535 ) {
                this->regs->CTRLA.bit.PRESCALER = index;
                this->regs->CC[0].reg = compare_value > 0 ? compare_value - 1 : 0;
                break;
            }
        }
        while(this->regs->SYNCBUSY.bit.CC0);

        this->regs->WAVE.bit.WAVEGEN = TC_WAVE_WAVEGEN_MFRQ_Val;

        this->regs->INTENSET.bit.MC0 = 1;
        NVIC_EnableIRQ(this->irqn);

        this->regs->CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;
        this->regs->CTRLA.bit.ENABLE = 1;
        while(this->regs->SYNCBUSY.bit.ENABLE);

        this->regs->CTRLBSET.bit.CMD = TC_CTRLBCLR_CMD_RETRIGGER_Val;
        while(this->regs->SYNCBUSY.bit.CTRLB);
    }

    /**
     * Stops timer.
     */
    void stop()
    {
        this->regs->CTRLA.bit.ENABLE = 0;
        while(this->regs->SYNCBUSY.bit.ENABLE);

        this->regs->INTENCLR.bit.MC0 = 1;
        NVIC_DisableIRQ(this->irqn);
        NVIC_ClearPendingIRQ(this->irqn);
    }

    /**
     * Restarts timer.
     */
    void restart()
    {
        this->regs->CTRLBSET.bit.CMD = TC_CTRLBCLR_CMD_RETRIGGER_Val;
        while(this->regs->SYNCBUSY.bit.CTRLB);
    }

    /**
     * Attach interrupt handler
     */
    void attachInterrupt(void (*isrCallback)(void*), void* context = nullptr)
    {
        this->isrContext = context;
        this->isrCallback = isrCallback;
        this->start();
    }
    void detachInterrupt()
    {
        this->stop();
        this->isrCallback = nullptr;
        this->isrContext = nullptr;
    }

private:
    std::uint_least8_t tcUnit;
    TcCount16* regs;
    IRQn_Type irqn;
    GCLKDescription gclk = GCLK_1M;
    std::uint64_t nanoseconds;
    void (*isrCallback)(void* context);
    void* isrContext = nullptr;

    void configureClock(std::uint8_t gclk_id)
    {
        switch(this->tcUnit) {
            case 0: {MCLK->APBAMASK.bit.TC0_ = 1; GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 1: {MCLK->APBAMASK.bit.TC1_ = 1; GCLK->PCHCTRL[TC1_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 2: {MCLK->APBBMASK.bit.TC2_ = 1; GCLK->PCHCTRL[TC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 3: {MCLK->APBBMASK.bit.TC3_ = 1; GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 4: {MCLK->APBCMASK.bit.TC4_ = 1; GCLK->PCHCTRL[TC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 5: {MCLK->APBCMASK.bit.TC5_ = 1; GCLK->PCHCTRL[TC5_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 6: {MCLK->APBDMASK.bit.TC6_ = 1; GCLK->PCHCTRL[TC6_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
            case 7: {MCLK->APBDMASK.bit.TC7_ = 1; GCLK->PCHCTRL[TC7_GCLK_ID].reg = GCLK_PCHCTRL_GEN(gclk_id) | GCLK_PCHCTRL_CHEN;  break;}
        }
    }
    void processIsr()
    {
        if( this->regs->INTFLAG.bit.MC0 ) {
            this->regs->INTFLAG.bit.MC0 = 1;
            if( this->isrCallback != nullptr ) {
                this->isrCallback(this->isrContext);
            }
        }
    }

    friend class SAMD51TCInterruptHelper;
};

extern SAMD51TC TimerTC0;
extern SAMD51TC TimerTC1;
extern SAMD51TC TimerTC2;
extern SAMD51TC TimerTC3;
extern SAMD51TC TimerTC4;
extern SAMD51TC TimerTC5;
extern SAMD51TC TimerTC6;
extern SAMD51TC TimerTC7;

#endif // SAMD51_TIMER_HPP__