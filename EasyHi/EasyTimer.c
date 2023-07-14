#include "EasyTimer.h"
#include "hi_early_debug.h"
#include "hi3861.h"
#include "hi_clock.h"
#include "EasyIsr.h"

/**
 * @brief the callback function of timer interrupt
 *
 */
__attribute__((weak)) void EasyTimer_CallbackFunc(Typedef_EasyTimerInit *EasyTimer)
{
    // remember to reset the flag of Isr
}

/**
 * @brief Initialize the hardware timer
 *
 * @param TimerStructure the configurations of Timer, want more please go to "EasyTimer.h"
 * @return EasyHi_Error: 0 for success, others go to "EasyError.h"
 */
EasyHi_Error EasyTimer_Init(Typedef_EasyTimerInit *TimerStructure)
{
    // read the config of timer
    hi_u32 Temp = 0x00,
           Setting = 0x00;
    hi_u16 Reg_Setting = 0;
    EasyHi_Error Ack = 0;
    Typedef_EasyIsrInit EasyTimerNVIC = {0};

    // enable timer clock
    hi_reg_read16(TIMER_REGISTER, Reg_Setting);
    if (TimerStructure->DEVICE == TIMER0)
        Reg_Setting |= (1 << 6);
    else if (TimerStructure->DEVICE == TIMER1)
        Reg_Setting |= (1 << 7);
    else if (TimerStructure->DEVICE == TIMER2)
        Reg_Setting |= (1 << 8);
    else if (TimerStructure->DEVICE == TIMER3)
        Reg_Setting |= (1 << 9);
    // else if (TimerStructure->DEVICE == RTC0)
    //     Reg_Setting |= (1 << 10);
    // else if (TimerStructure->DEVICE == RTC1)
    //     Reg_Setting |= (1 << 11);
    // else if (TimerStructure->DEVICE == RTC2)
    //     Reg_Setting |= (1 << 12);
    // else if (TimerStructure->DEVICE == RTC3)
    //     Reg_Setting |= (1 << 13);
    hi_reg_write16(TIMER_REGISTER, Reg_Setting);

    // hi_reg_setbitmsk(TIMER_RAWINTSTATUS, 0x0000000f);

    // disable the timer
    hi_reg_write32(TimerStructure->DEVICE + CONTROLREG, 0);
    hi_reg_read32(TimerStructure->DEVICE + CONTROLREG, Temp);
    if ((Temp & 0x00000001) == 0x000000001)
        return TIMER_STOPFAIL;

    // read the control register of timer
    if (TimerStructure->ITMASK == ITMASK_ENABLE)
        Setting |= 1U << 2;
    if (TimerStructure->MODE == MODE_CYCLE)
    {
        Setting |= 1U << 1;

        // set the timer to cycle mode and ITMASK
        hi_reg_write32(TimerStructure->DEVICE + CONTROLREG, Setting);
        hi_reg_read32(TimerStructure->DEVICE + CONTROLREG, Temp);
    }
    else
    {
        // set the load counter to 0xffffffff
        hi_reg_write32(TimerStructure->DEVICE + LOADCOUNT, 0xFFFFFFFF);
        hi_reg_read32(TimerStructure->DEVICE + LOADCOUNT, Temp);
        if (Temp != 0xFFFFFFFF)
            return TIMERLOAD_SETFAIL;

        // set timer to free mode and ITMASK
        hi_reg_write32(TimerStructure->DEVICE + CONTROLREG, Setting);
        hi_reg_read32(TimerStructure->DEVICE + CONTROLREG, Temp);
    }
    if ((Temp & 0x0000000F) != Setting)
        return TIMERCONTROL_SETFAIL;

    // set the load counter register of timer
    if (TimerStructure->LOAD == TIMER_GETFREQFAIL)
        return TIMERLOAD_SETFAIL;
    hi_reg_write32(TimerStructure->DEVICE + LOADCOUNT, TimerStructure->LOAD);
    hi_reg_read32(TimerStructure->DEVICE + LOADCOUNT, Temp);
    if (Temp != TimerStructure->LOAD)
        return TIMERLOAD_SETFAIL;

    // Initialize the Isr of Timer
    if (TimerStructure->NVIC == NVIC_ENABLE && TimerStructure->DEVICE != TIMER3)
    {
        // set device
        if (TimerStructure->DEVICE == TIMER0)
            EasyTimerNVIC.DEVICE = TIMER_0_IRQ;
        else if (TimerStructure->DEVICE == TIMER1)
            EasyTimerNVIC.DEVICE = TIMER_1_IRQ;
        else if (TimerStructure->DEVICE == TIMER2)
            EasyTimerNVIC.DEVICE = TIMER_2_IRQ;

        // set other configurations
        EasyTimerNVIC.PRIORITY = TimerStructure->NVIC_PRIORITY;
        EasyTimerNVIC.CALLBACKFUNC = (irq_routine)EasyTimer_CallbackFunc;
        EasyTimerNVIC.CALLBACKPARA = TimerStructure;

        Ack = EasyIsr_Init(&EasyTimerNVIC);
        if (Ack != EasyHi_SUCCESS)
            return Ack;
    }

    return EasyHi_SUCCESS;
}

/**
 * @brief Start the hardware Timer
 *
 * @param TimerStructure
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_Start(Typedef_EasyTimerInit *TimerStructure)
{
    unsigned long Setting = 0x00;

    // enable the timer
    hi_reg_setbitmsk(TimerStructure->DEVICE + CONTROLREG, 0x00000001);
    hi_reg_read32(TimerStructure->DEVICE + CONTROLREG, Setting);
    if ((Setting & 0x00000001) != 0x000000001)
        return TIMER_STARTFAIL;

    return EasyHi_SUCCESS;
}

/**
 * @brief Stop the hardware Timer
 *
 * @param TimerStructure
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_Stop(Typedef_EasyTimerInit *TimerStructure)
{
    unsigned long Setting = 0x00;

    // disable the timer
    hi_reg_clrbitmsk(TimerStructure->DEVICE + CONTROLREG, 0x00000001);
    hi_reg_read32(TimerStructure->DEVICE + CONTROLREG, Setting);
    if ((Setting & 0x00000001) == 0x000000001)
        return TIMER_STOPFAIL;

    return EasyHi_SUCCESS;
}

/**
 * @brief Start the interrupt of Timer
 *
 * @param TimerStructure
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_ITStart(Typedef_EasyTimerInit *TimerStructure)
{
    unsigned char Timer = 0;
    EasyHi_Error Ack = 0;

    if (TimerStructure->DEVICE == TIMER0)
        Timer = TIMER_0_IRQ;
    else if (TimerStructure->DEVICE == TIMER1)
        Timer = TIMER_1_IRQ;
    else if (TimerStructure->DEVICE == TIMER2)
        Timer = TIMER_2_IRQ;
    else
        return TIMERNVIC_STARTFAIL;

    Ack = hi_irq_enable(Timer);
    if (Ack != EasyHi_SUCCESS)
        return TIMERNVIC_STARTFAIL;
    return EasyHi_SUCCESS;
}

/**
 * @brief Stop the interrupt of Timer
 *
 * @param TimerStructure
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_ITStop(Typedef_EasyTimerInit *TimerStructure)
{
    unsigned char Timer = 0;

    if (TimerStructure->DEVICE == TIMER0)
        Timer = TIMER_0_IRQ;
    else if (TimerStructure->DEVICE == TIMER1)
        Timer = TIMER_1_IRQ;
    else if (TimerStructure->DEVICE == TIMER2)
        Timer = TIMER_2_IRQ;
    else
        return TIMERNVIC_STOPFAIL;

    hi_irq_disable(Timer);
    return EasyHi_SUCCESS;
}

/**
 * @brief Clear the interrupt flag of Timer
 *
 * @param TimerStructure
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_ClearIsrFlag(Typedef_EasyTimerInit *TimerStructure)
{
    unsigned long Setting = 0x00;

    EasyIsr_DisableAll();
    // clear the Isr flag of timer
    hi_reg_read_val32(TimerStructure->DEVICE + EOI);
    hi_reg_read32(TimerStructure->DEVICE + EOI, Setting);
    if ((Setting & 0x00000001) == 0x00000001)
    {
        EasyIsr_EnableAll();
        return TIMERNVIC_CLEARFAIL;
    }

    EasyIsr_EnableAll();
    return EasyHi_SUCCESS;
}

/**
 * @brief Get the frequency of clock
 *
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_GetClockFreq(void)
{
    hi_u32 Frequency = 0;
    Frequency = hi_get_xtal_clock();

    switch (Frequency)
    {
    case HI_XTAL_CLOCK_24M:
        return HI_XTAL_24MHZ_VAL;
        break;
    case HI_XTAL_CLOCK_40M:
        return HI_XTAL_40MHZ_VAL;
        break;
    default:
        return TIMER_GETFREQFAIL;
        break;
    }
}

/**
 * @brief Set count load of timer
 *
 * @param TimerStructure
 * @return EasyHi_Error
 */
EasyHi_Error EasyTimer_SetLoad(Typedef_EasyTimerInit *TimerStructure)
{
    hi_u32 Temp = 0;

    // set the load counter register of timer
    if (TimerStructure->LOAD == TIMER_GETFREQFAIL)
        return TIMERLOAD_SETFAIL;
    hi_reg_write32(TimerStructure->DEVICE + LOADCOUNT, TimerStructure->LOAD);
    hi_reg_read32(TimerStructure->DEVICE + LOADCOUNT, Temp);
    if (Temp != TimerStructure->LOAD)
        return TIMERLOAD_SETFAIL;

    return EasyHi_SUCCESS;
}

/**
 * @brief Get the current value of timer count
 * 
 * @param TimerStructure 
 * @return EasyHi_Error 
 */
EasyHi_Error EasyTimer_GetCurrentValue(Typedef_EasyTimerInit *TimerStructure)
{
    hi_u32 Value = 0;

    hi_reg_read32(TimerStructure->DEVICE + CURRENTVALUE, Value);
    return Value;
}
