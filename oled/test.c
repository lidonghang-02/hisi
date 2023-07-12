#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "oled.h"
static void oled(void)
{
    OLED_Init();
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0); // 0正常显示 1 屏幕翻转显示

    /*
    状(0) 态(1) 停(2) 机(3) 运(4) 行(5) 中(6)
    已(7) 铲(8) 猫(9) 屎(10)
    剩(11) 余(12) 猫(13) 砂(14)
    今(15) 日(16) 铲(17) 屎(18) 次(19) 数(20)
    :(21)  "  "(22)

    */

    // 状态：
    OLED_ShowChinese(0, 0, 0, 12, 1);
    OLED_ShowChinese(14, 0, 1, 12, 1);
    OLED_ShowChinese(28, 0, 21, 12, 1);

    // 判断状态
    if (1)
    {
        OLED_ShowChinese(42, 0, 2, 12, 1);
        OLED_ShowChinese(56, 0, 3, 12, 1);
        OLED_ShowChinese(70, 0, 22, 12, 1);
    }
    else
    {
        OLED_ShowChinese(42, 0, 4, 12, 1);
        OLED_ShowChinese(56, 0, 5, 12, 1);
        OLED_ShowChinese(70, 0, 6, 12, 1);
    }

    /*输出1kg*/
    OLED_ShowString(84, 0, "1kg", 12, 1); // 6*8 “ABC”

    // 已铲猫屎：
    OLED_ShowChinese(0, 14, 7, 12, 1);
    OLED_ShowChinese(14, 14, 8, 12, 1);
    OLED_ShowChinese(28, 14, 9, 12, 1);
    OLED_ShowChinese(42, 14, 10, 12, 1);
    OLED_ShowChinese(56, 14, 21, 12, 1);

    // 剩余猫砂：
    OLED_ShowChinese(0, 28, 11, 12, 1);
    OLED_ShowChinese(14, 28, 12, 12, 1);
    OLED_ShowChinese(28, 28, 13, 12, 1);
    OLED_ShowChinese(42, 28, 14, 12, 1);
    OLED_ShowChinese(56, 28, 21, 12, 1);

    // 今日铲屎次数：
    OLED_ShowChinese(0, 42, 15, 12, 1);
    OLED_ShowChinese(14, 42, 16, 12, 1);
    OLED_ShowChinese(28, 42, 17, 12, 1);
    OLED_ShowChinese(42, 42, 18, 12, 1);
    OLED_ShowChinese(56, 42, 19, 12, 1);
    OLED_ShowChinese(70, 42, 20, 12, 1);
    OLED_ShowChinese(84, 42, 21, 12, 1);
    // 输出数字（次数）
    OLED_ShowChinese(112, 42, 19, 12, 1);

    OLED_Refresh();
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
