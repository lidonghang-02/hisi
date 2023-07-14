#ifndef __STEPMOTOR_H__
#define __STEPMOTOR_H__

#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "cmsis_os2.h"
#include "stdio.h"
#include "unistd.h"


#define Positive (1)
#define Negative (-1)

// 定义步进电机IO端口及其复用
#define StepMotor_PinA  9
#define StepMotor_PinB  12
#define StepMotor_PinC  13
#define StepMotor_PinD  14
#define StepMotor_PinAFunc  IOT_GPIO_FUNC_GPIO_9_GPIO
#define StepMotor_PinBFunc  IOT_GPIO_FUNC_GPIO_12_GPIO
#define StepMotor_PinCFunc  IOT_GPIO_FUNC_GPIO_13_GPIO
#define StepMotor_PinDFunc  IOT_GPIO_FUNC_GPIO_14_GPIO

// 单独开启步进电机某相的信号
#define StepMotorPinA_ON IoTGpioSetOutputVal(StepMotor_PinA, IOT_GPIO_VALUE0)
#define StepMotorPinB_ON IoTGpioSetOutputVal(StepMotor_PinB, IOT_GPIO_VALUE0)
#define StepMotorPinC_ON IoTGpioSetOutputVal(StepMotor_PinC, IOT_GPIO_VALUE0)
#define StepMotorPinD_ON IoTGpioSetOutputVal(StepMotor_PinD, IOT_GPIO_VALUE0)

#define StepMotorPinA_OFF IoTGpioSetOutputVal(StepMotor_PinA, IOT_GPIO_VALUE1)
#define StepMotorPinB_OFF IoTGpioSetOutputVal(StepMotor_PinB, IOT_GPIO_VALUE1)
#define StepMotorPinC_OFF IoTGpioSetOutputVal(StepMotor_PinC, IOT_GPIO_VALUE1)
#define StepMotorPinD_OFF IoTGpioSetOutputVal(StepMotor_PinD, IOT_GPIO_VALUE1)

typedef enum
{
    FOR = 0,
    REW = 1,
    OFF
} MotorStatus;

// 关闭步进电机的输入信号
#define StepMotorPin_OFF do{    \
IoTGpioSetOutputVal(StepMotor_PinA, IOT_GPIO_VALUE1);   \
IoTGpioSetOutputVal(StepMotor_PinB, IOT_GPIO_VALUE1);   \
IoTGpioSetOutputVal(StepMotor_PinC, IOT_GPIO_VALUE1);   \
IoTGpioSetOutputVal(StepMotor_PinD, IOT_GPIO_VALUE1);   \
}while(0)

void StepMotor_Init(void);
void StepMotor_SetStatus(uint8_t motor_status);
void StepMotor_InitPin(uint8_t StepMotor_Pin, uint8_t StepMotor_PinFunc);
void StepMotor_Test(void);
void StepMotor_Run(MotorStatus status);

#endif