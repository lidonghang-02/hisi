#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "ohos_types.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "hi_time.h"
#include "iot_watchdog.h"
#include "hi_io.h"
#include "../include/HCSR04.h"
#include "../../EasyHi/EasyIsr.h"

void Hcsr04_Init(void)
{
    printf("HCSR04 init\r\n");
    IoTGpioInit(Trig);
    IoTGpioSetFunc(Trig, IOT_GPIO_FUNC_GPIO_7_GPIO);
    IoTGpioSetDir(Trig, IOT_GPIO_DIR_OUT);

    IoTGpioInit(Echo);
    IoTGpioSetFunc(Echo, IOT_GPIO_FUNC_GPIO_8_GPIO);
    IoTGpioSetDir(Echo, IOT_GPIO_DIR_IN);
}

float GetDistance(void)
{
    // printf("HCSR04 GetDistance\r\n");
    EasyIsr_DisableAll();
    static unsigned long start_time = 0, time = 0;
    float dis = 0.0;
    unsigned int flag = 0, stop = 40000;
    IotGpioValue value = IOT_GPIO_VALUE0;

    IoTGpioSetOutputVal(Trig, IOT_GPIO_VALUE0);
    IoTGpioSetOutputVal(Trig, IOT_GPIO_VALUE1);
    hi_udelay(20);
    // usleep(20);
    IoTGpioSetOutputVal(Trig, IOT_GPIO_VALUE0);
    while (stop)
    {
        stop--;
        IoTGpioGetInputVal(Echo, &value);
        if (value == IOT_GPIO_VALUE1 && flag == 0)
        {
            start_time = hi_get_us();
            flag = 1;
        }
        if (value == IOT_GPIO_VALUE0 && flag == 1)
        {
            time = hi_get_us() - start_time;
            break;
        }
    }
    dis = time * 0.034 / 2;
    EasyIsr_EnableAll();
    return dis;
}