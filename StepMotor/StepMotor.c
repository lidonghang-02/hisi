#include "StepMotor.h"
#include "EasyTimer.h"
#include "EasyIsr.h"
#include "cmsis_os2.h"
#include <stdlib.h>

// #define Speed_Max

osTimerId_t Timer_ID = 0;
int8_t direction = 0;
Typedef_EasyTimerInit StepMotor_Timer = {0};
Typedef_EasyIsrInit StepMotor_Isr = {0};

/**
 * @brief 初始化电机IO口
 *
 * @param StepMotor_Pin 所要初始化的IO
 */
void StepMotor_InitPin(uint8_t StepMotor_Pin, uint8_t StepMotor_PinFunc)
{
    IoTGpioInit(StepMotor_Pin);
    IoTGpioSetFunc(StepMotor_Pin, StepMotor_PinFunc);
    IoTGpioSetDir(StepMotor_Pin, IOT_GPIO_DIR_OUT);
}

void StepMotor_CallbackFunc(void)
{
    static int8_t count = 0;

    StepMotor_SetStatus(count);

    if (direction == Positive)
    {
        if (++count >= 8)
            count = 0;
    }
    else if (direction == Negative)
    {
        if (--count < 0)
            count = 7;
    }

    // printf("Get into the IT, count = %d\n", count);
    EasyTimer_ClearIsrFlag(&StepMotor_Timer);
}

// void StepMotor_ControlCallbackFunc(void)
// {
// }

/**
 * @brief 电机初始化，包括IO和定时器
 *
 */
void StepMotor_Init(void)
{
    // Gpio set high level
    StepMotorPin_OFF;

    // Init Gpio
    StepMotor_InitPin(StepMotor_PinA, StepMotor_PinAFunc);
    StepMotor_InitPin(StepMotor_PinB, StepMotor_PinBFunc);
    StepMotor_InitPin(StepMotor_PinC, StepMotor_PinCFunc);
    StepMotor_InitPin(StepMotor_PinD, StepMotor_PinDFunc);

    // initialize hardware timer to control the operation of step motor
    StepMotor_Timer.DEVICE = TIMER1;
    StepMotor_Timer.ITMASK = ITMASK_DISABLE;
    StepMotor_Timer.LOAD = EasyTimer_GetClockFreq();
    StepMotor_Timer.MODE = MODE_CYCLE;
    StepMotor_Timer.NVIC = NVIC_DISABLE;

    EasyHi_Error ack = EasyTimer_Init(&StepMotor_Timer);
    if (ack != EasyHi_SUCCESS)
        printf("Timer Error:%d\n", ack);
    else
        printf("StepMotor Timer Init Successfully!\n");

    // initialize the interrupt of hardware timer
    StepMotor_Isr.CALLBACKFUNC = StepMotor_CallbackFunc;
    StepMotor_Isr.CALLBACKPARA = NULL;
    StepMotor_Isr.DEVICE = TIMER_1_IRQ;
    StepMotor_Isr.PRIORITY = 7;

    ack = EasyIsr_Init(&StepMotor_Isr);
    if (ack != EasyHi_SUCCESS)
        printf("Isr Error:%d\n", ack);
    else
        printf("StepMotor Isr Init Successfully!\n");

    // // initialize the osTimer to control the speed of step motor
    // Timer_ID = osTimerNew(StepMotor_ControlCallbackFunc, osTimerPeriodic, NULL, NULL);
    // if (Timer_ID == NULL)
    //     printf("osTimer Init Fail\n");
    // else
    //     printf("osTimer Init Successfully\n");
}

void StepMotor_SetStatus(uint8_t motor_status)
{
    switch (motor_status)
    {
    case 0:
        StepMotorPin_OFF;
        StepMotorPinA_ON;
        break;
    case 1:
        StepMotorPin_OFF;
        StepMotorPinA_ON;
        StepMotorPinB_ON;
        break;
    case 2:
        StepMotorPin_OFF;
        StepMotorPinB_ON;
        break;
    case 3:
        StepMotorPin_OFF;
        StepMotorPinB_ON;
        StepMotorPinC_ON;
        break;
    case 4:
        StepMotorPin_OFF;
        StepMotorPinC_ON;
        break;
    case 5:
        StepMotorPin_OFF;
        StepMotorPinC_ON;
        StepMotorPinD_ON;
        break;
    case 6:
        StepMotorPin_OFF;
        StepMotorPinD_ON;
        break;
    case 7:
        StepMotorPin_OFF;
        StepMotorPinD_ON;
        StepMotorPinA_ON;
        break;
    default:
        printf("[StepMotor]Wrong Status!\n");
        break;
    }
    // printf("StepMotor_Status = %d\n", motor_status);
}

void StepMotor_SetSpeed(uint32_t cycle, int8_t direct)
{
    StepMotor_Timer.LOAD = cycle;
    EasyTimer_SetLoad(&StepMotor_Timer);
    direction = direct;
}

void StepMotor_Run(MotorStatus status)
{
    switch (status)
    {
    case FOR:
        StepMotor_SetSpeed(40000, Positive);
        EasyTimer_Start(&StepMotor_Timer);
        EasyIsr_Start(&StepMotor_Isr);
        printf("Motor Run Positive\n");
        break;
    case REW:
        StepMotor_SetSpeed(40000, Negative);
        EasyTimer_Start(&StepMotor_Timer);
        EasyIsr_Start(&StepMotor_Isr);
        printf("Motor Run Negative\n");
        break;
    case OFF:
        EasyIsr_Stop(&StepMotor_Isr);
        EasyTimer_Stop(&StepMotor_Timer);
        break;
    default:
        break;
    }
}

void StepMotor_Test(void)
{
    if (EasyTimer_Start(&StepMotor_Timer) != EasyHi_SUCCESS)
        printf("Timer Start Fail\n");
    if (EasyIsr_Start(&StepMotor_Isr) != EasyHi_SUCCESS)
        printf("Isr Start Fail\n");
}