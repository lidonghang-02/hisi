#ifndef __EASYTIMER_H__
#define __EASYTIMER_H__

#include "EasyError.h"
#include "hi_types_base.h"

/**
 * @brief The setting of Timer
 *
 */
#define ITMASK_ENABLE 0
#define ITMASK_DISABLE 1

#define MODE_FREE 2
#define MODE_CYCLE 3

#define NVIC_ENABLE 4
#define NVIC_DISABLE 5

/**
 * @brief the address of hardware Timer1, and Timern is add 14*n to it.
 *
 */
#define TIMER_BASEADDR (0x40050000)
#define RTC_BASEADDR (0x50007000)
#define TIMER_REGISTER (0x40010030)
#define TIMER_RAWINTSTATUS (TIMER_BASEADDR + 0xA8)
#define TIMER_OFFSET (0x14)
#define LOADCOUNT (0x000)
#define CURRENTVALUE (0x004)
#define CONTROLREG (0x008)
#define EOI (0x00C)
#define INTSTATUS (0x010)

#define TIMER0 (TIMER_BASEADDR + TIMER_OFFSET * 0)
#define TIMER1 (TIMER_BASEADDR + TIMER_OFFSET * 1)
#define TIMER2 (TIMER_BASEADDR + TIMER_OFFSET * 2)
#define TIMER3 (TIMER_BASEADDR + TIMER_OFFSET * 3)

#define RTC0 (RTC_BASEADDR + TIMER_OFFSET * 0) /* Not Supported Yet*/
#define RTC1 (RTC_BASEADDR + TIMER_OFFSET * 1) /* Not Supported Yet*/
#define RTC2 (RTC_BASEADDR + TIMER_OFFSET * 2) /* Not Supported Yet*/
#define RTC3 (RTC_BASEADDR + TIMER_OFFSET * 3) /* Not Supported Yet*/

typedef struct EasyTimer_Init
{
    hi_u32 DEVICE;                 /* TIMERx  x: 0 ~ 3, but TIMER3 has no NVIC, and TIMER1 may be used by system */
    hi_u32 LOAD;                   /* the degressive count load of timer at most 0xffffffff */
    hi_u8 MODE;                    /* the mode of Timer: MODE_FREE and MODE_CYCLE to reload the count load automatically */
    hi_u8 NVIC;                    /* whether to enable NVIC: NVIC_ENABLE or NVIC_DISABLE */
    hi_u8 ITMASK;                  /* whether to enable the mask of interrupt: ITMASK_ENABLE or ITMASK_DISABLE */
    hi_u8 NVIC_PRIORITY;           /* the priority of NVIC, from 1 to 7 */
} Typedef_EasyTimerInit;

EasyHi_Error EasyTimer_Init(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_Start(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_Stop(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_ITStart(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_ITStop(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_ClearIsrFlag(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_GetClockFreq(void);
EasyHi_Error EasyTimer_SetLoad(Typedef_EasyTimerInit *TimerStructure);
EasyHi_Error EasyTimer_GetCurrentValue(Typedef_EasyTimerInit *TimerStructure);

#endif