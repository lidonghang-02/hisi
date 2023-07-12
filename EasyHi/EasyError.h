#ifndef __EASYERROR_H__
#define __EASYERROR_H__

#include <hi_errno.h>

/**
 * @brief the error code of easytimer
 *
 */
typedef unsigned long EasyHi_Error;

// the details about error codes of EasyHi
#define EasyHi_SUCCESS (0)        /* function succeed to operate */
#define TIMERLOAD_SETFAIL (-2)    /* the timer set load fail */
#define TIMERCONTROL_SETFAIL (-3) /* the timer set control fail */
#define TIMER_STARTFAIL (-4)      /* timer start fail */
#define TIMER_STOPFAIL (-5)       /* timer stop fail */
#define TIMER_GETFREQFAIL (-6)    /* timer get wrong data about frequency of clock */
#define TIMERNVIC_CLEARFAIL (-7)   /* timer clear the flag of Isr fail*/
#define TIMERNVIC_STARTFAIL (-8)
#define TIMERNVIC_STOPFAIL (-9)
#define TIMERVALUE_SETFAIL (-10)
#define ISR_DEVICEERROR (-20)
#define ISR_PRIORITYERROR (-21)

#endif