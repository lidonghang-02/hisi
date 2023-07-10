#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "oled.h"
static void oled(void)
{
    OLED_Main();
}
static void oledTask(void)
{
    osThreadAttr_t attr;
    attr.name = "oled";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1012; // 堆栈大小为 1012, 如果发生栈溢出则需增大
    attr.priority = osPriorityNormal;
    if (osThreadNew((osThreadFunc_t)oled, NULL, &attr) == NULL)
    {
        printf("[oled] Failed to create oledTask!\n");
    }
}
APP_FEATURE_INIT(oledTask);
