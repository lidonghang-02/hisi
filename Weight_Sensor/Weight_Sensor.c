#include "Weight_Sensor.h"
#include "hi_time.h"
#include "hi_isr.h"
#include "stdio.h"
#include "../LitterCleaner.h"

#define DTSet(__Value__) IoTGpioSetOutputVal(WS_DT, __Value__)
#define SCKSet(__Value__) IoTGpioSetOutputVal(WS_SCK, __Value__)
#define DTGet(__Value__) IoTGpioGetInputVal(WS_DT, __Value__)

void WS_Init(void)
{
    IoTGpioInit(WS_DT);
    IoTGpioInit(WS_SCK);
    IoTGpioSetFunc(WS_DT, WS_DTFunc);
    IoTGpioSetFunc(WS_SCK, WS_SCKFunc);
    IoTGpioSetDir(WS_DT, IOT_GPIO_DIR_IN);
    IoTGpioSetDir(WS_SCK, IOT_GPIO_DIR_OUT);
    IoTGpioSetPull(WS_DT, IOT_GPIO_PULL_UP);
    IoTGpioSetOutputVal(WS_DT, IOT_GPIO_VALUE1);
    IoTGpioSetOutputVal(WS_SCK, IOT_GPIO_VALUE1);
}

unsigned long WS_Read(void)
{
    unsigned long data = 0x00;
    IotGpioValue val = 0;
    hi_u32 status = 0;

    status = hi_int_lock();
    // IoTGpioUnregisterIsrFunc(12);

    DTSet(1);
    hi_udelay(1);
    SCKSet(0);
    do
    {
        DTGet(&val);
    } while (val);

    for (unsigned char i = 0; i < 24; i++)
    {
        SCKSet(1);
        data = data << 1;
        hi_udelay(1);
        SCKSet(0);
        hi_udelay(1);
        DTGet(&val);
        if (val)
            data++;
    }

    SCKSet(1);
    hi_udelay(1);
    data ^= 0x800000;
    SCKSet(0);
    hi_udelay(1);

    hi_int_restore(status);
    // IoTGpioRegisterIsrFunc(12, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LitterCleaner_KeyCallback, NULL);

    return data;
}