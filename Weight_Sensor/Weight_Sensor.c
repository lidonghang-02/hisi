#include "Weight_Sensor.h"
#include "hi_time.h"

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
    IoTGpioSetOutputVal(WS_DT, IOT_GPIO_VALUE1);
    IoTGpioSetOutputVal(WS_SCK, IOT_GPIO_VALUE1);
}

unsigned long WS_Read(void)
{
    unsigned long data = 0x00;
    IotGpioValue val = 0;

    DTSet(1);
    SCKSet(1);
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
            data |= 0x01;
    }

    SCKSet(1);
    hi_udelay(1);
    data ^= 0x800000;
    SCKSet(0);
    hi_udelay(1);

    return data;
}