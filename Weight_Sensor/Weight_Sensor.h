#ifndef __WEIGHT_SENSOR_H__
#define __WEIGHT_SENSOR_H__

#include "iot_gpio.h"
#include "iot_gpio_ex.h"

#define WS_DT 12
#define WS_SCK 10
#define WS_DTFunc IOT_GPIO_FUNC_GPIO_12_GPIO
#define WS_SCKFunc IOT_GPIO_FUNC_GPIO_10_GPIO

unsigned long WS_Read(void);
void WS_Init(void);

#endif