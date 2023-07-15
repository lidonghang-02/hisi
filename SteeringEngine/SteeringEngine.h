#ifndef __STEERINGENGINE_H__
#define __STEERINGENGINE_H__

#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "cmsis_os2.h"
#include "stdio.h"
#include "unistd.h"

/**
 * @brief the IO of Steering Engine,including its function
 * 
 */
#define SteeringEngine_IO 10
#define SteeringEngine_IOFunc IOT_GPIO_FUNC_GPIO_10_GPIO

/**
 * @brief the io function of Steering Engine
 * 
 */
#define SteeringEngine_IOPullUp IoTGpioSetOutputVal(SteeringEngine_IO, IOT_GPIO_VALUE1)
#define SteeringEngine_IOPullDown IoTGpioSetOutputVal(SteeringEngine_IO, IOT_GPIO_VALUE0)

void SteeringEngine_Init(void);
void SteeringEngine_SetAngleOnce(uint16_t duty);
void SteeringEngine_SetAngle(uint8_t angle);

#endif