#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "hi_watchdog.h"
#include "iot_gpio.h"
// #include "iot_gpio_ex.h"
#include "hi_wifi_api.h"
#include "hi_nv.h"

#include "wifi_config/ap_mode.h"
#include "wifi_config/sta_mode.h"
#include "oled/include/oled.h"
#include "iot_cloud_oc/include/wifi_connect.h"

#include "Weight_Sensor/Weight_Sensor.h"
#include "StepMotor/StepMotor.h"
#include "SteeringEngine/SteeringEngine.h"
#include "HCSR04.h"
#include "iot_cloud.h"

// 按键检测相关设置
#define KEYLONGTIME 150
#define KEYCLICKTIME 12
#define Key_MaxClick 3

// wifi的nv地址
#define WIFI_NVID 0x0B

// 保护距离
#define Prot_Distance 20

// 重复测量次数
#define ReWeighTimes 5

// 铲屎变化重量阈值
#define LitterPressure 100

typedef enum
{
    Key_Down = 0,
    Key_Up,
} KeyStatus;

typedef struct
{
    hi_u8 ssid[33];
    hi_u8 passwd[65];
} Wifi_InitInfo;

// 按键状态
struct LitterCleaner_Key
{
    KeyStatus key_status;
    uint32_t key_last;
    uint16_t key_click;
} Key = {1, 0, 0};

// 按键状态机的软件定时器
osTimerId_t KeyTimer_ID = 0;

// 按键定时器
osTimerId_t StepMotor_KeyID = 0;
// 防夹猫标志位
uint8_t Protec_flag = 0;
// 电机运转标识
MotorStatus StepMotor_Status = OFF;
// 铲屎信号
int Operation_Sign = 0;

float distance = 0;                   // 距离
unsigned long long pressure = 0;      // 变化重量
unsigned long long pressure_init = 0; // 初始化重量
float pressure_delta = 0;
float pressure_litter = 0; // 猫砂重量
float pressure_shift = 0;  // 铲屎重量

/**
 * @brief 按键外部中断回调函数
 *
 */
// void LitterCleaner_KeyCallback(void)
// {
//     Key.key_status = Key_Down;

//     osTimerStart(KeyTimer_ID, 1);

//     // printf("Get into the key interrupt function\n");
// }

/**
 * @brief 按键中断状态机
 *
 */
void LitterCleaner_KeyRegconize(void)
{
    static uint16_t wait_time = 0;

    IoTGpioGetInputVal(11, &Key.key_status);

    if (Key.key_status == Key_Down)
    {
        Key.key_last++;

        if (wait_time > KEYCLICKTIME)
        {
            wait_time = 0;
            Key.key_click++;
        }

        if (wait_time != 0 && wait_time <= KEYCLICKTIME)
        {
            Key.key_click++;
            wait_time = 0;
        }
    }
    else if (Key.key_status == Key_Up)
    {
        wait_time++;
        if (wait_time > KEYCLICKTIME)
        {
            Key.key_click = 0;
            // osTimerStop(KeyTimer_ID);
        }
        Key.key_last = 0;
    }
    else
    {
        wait_time = 0;
        Key.key_click = 0;
        Key.key_last = 0;
        // osTimerStop(KeyTimer_ID);
    }

    // printf("Get into the key timer\n");
}

// 终点限位开关
void LimitedKey_1(void)
{
    printf("Key1 Down\n");
    IoTGpioUnregisterIsrFunc(14);
    StepMotor_Run(REW);
    StepMotor_Status = REW;
}

// 起点限位开关
void LimitedKey_2(void)
{
    printf("Key2 Down\n");
    StepMotor_Run(OFF);
    IoTGpioUnregisterIsrFunc(13);
    Operation_Sign = 0;
    StepMotor_Status = OFF;
}

void Key_PinInit(void)
{
    // 按键初始化
    IoTGpioInit(11);
    IoTGpioSetFunc(11, IOT_GPIO_FUNC_GPIO_11_GPIO);
    IoTGpioSetDir(11, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(11, IOT_GPIO_PULL_UP);
    IoTGpioSetOutputVal(11, IOT_GPIO_VALUE1);

    // IoTGpioRegisterIsrFunc(11, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LitterCleaner_KeyCallback, NULL);
}

/**
 * @brief 按键初始化，采用外部中断方式
 *
 */
static void LitterCleaner_KeyInit(void)
{
    Key_PinInit();
    KeyTimer_ID = osTimerNew(LitterCleaner_KeyRegconize, osTimerPeriodic, NULL, NULL);
    if (KeyTimer_ID == NULL)
        printf("KeyTimer Init Fail!\n");
    osTimerStart(KeyTimer_ID, 1);

    // 限位开关初始化
    IoTGpioInit(13);
    IoTGpioSetFunc(13, IOT_GPIO_FUNC_GPIO_13_GPIO);
    IoTGpioSetDir(13, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(13, IOT_GPIO_PULL_UP);
    IoTGpioSetOutputVal(13, IOT_GPIO_VALUE1);
    // IoTGpioRegisterIsrFunc(13, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LimitedKey_2, NULL);
    IoTGpioInit(14);
    IoTGpioSetFunc(14, IOT_GPIO_FUNC_GPIO_14_GPIO);
    IoTGpioSetDir(14, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(14, IOT_GPIO_PULL_UP);
    IoTGpioSetOutputVal(14, IOT_GPIO_VALUE1);
    // IoTGpioRegisterIsrFunc(14, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LimitedKey_1, NULL);
}

/**
 * @brief wifi初始化连接
 *
 */
static char LitterCleaner_WifiInit(void)
{
    Wifi_InitInfo wifi = {0};
    char ret = 0;

    printf("Ready to connect to wifi...\n");

    ret = hi_factory_nv_read(WIFI_NVID, &wifi, sizeof(Wifi_InitInfo), 0);
    if (ret == HISI_OK)
        printf("Succeed getting the wifi info\n");
    else
        return -1;

    // test init
    // strcpy_s(wifi.ssid, HI_WIFI_MAX_SSID_LEN, "MSI");
    // strcpy_s(wifi.passwd, HI_WIFI_MAX_KEY_LEN, "128215781");

    WifiConnect(&wifi.ssid, &wifi.passwd);

    if (ret == 0)
    {
        printf("[Wifi] Connection is sucessful!\n");
        return 0;
    }
    else
    {
        printf("[Wifi] Connection failed!\n");
        return -1;
    }
}

/**
 * @brief 按键检测
 *
 */
static void LitterCleaner_Key(void)
{
    char i = 0;
    hi_wifi_status wifi_info = {0};

    while (1)
    {
        if (Key.key_status == Key_Down && StepMotor_Status == OFF)
        {
            // printf("get into key task \n");
            // 松手检测
            for (i = 1; i < Key_MaxClick; i++)
            {
                while (Key.key_status == Key_Down && Key.key_last < KEYLONGTIME)
                    osDelay(1);
                while (Key.key_click == i && Key.key_last < KEYLONGTIME)
                    osDelay(1);
                if (Key.key_click == 0)
                    break;
            }

            if (Key.key_last >= KEYLONGTIME) // 长按
            {
                printf("Key Long\n");

                // 检测是否已连接wifi
                hi_wifi_sta_get_connect_info(&wifi_info);
                if (wifi_info.status == HI_WIFI_DISCONNECTED)
                    wifi_start_softap();
            }
            else if (i == 3) // 三击
            {
                printf("Key Click 3\n");
                if (!Protec_flag)
                {
                    SteeringEngine_SetAngle(90);
                    osDelay(500);
                    SteeringEngine_SetAngle(0);
                }

                else if (i == 2) // 双击
                {
                    printf("Key Double\n");
                    pressure_init = 0;
                    for (int i = 0; i < ReWeighTimes; i++)
                    {
                        pressure_init += WS_Read();
                        osDelay(10);
                    }
                }
                else // 单击
                {
                    printf("Key Down\n");
                }
            }

            osDelay(10);
        }
    }
}

static void LitterCleaner_Sensor(void)
{
    // 剩余猫砂， 已铲猫屎， 工作次数
    static int Litter = 0, Shift = 0, cnt = 0;
    static unsigned char count = 0;
    uint8_t str[64];

    app_msg_t *app_msg;

    OLED_Main();
    while (1)
    {
        hi_watchdog_feed();
        // 读取数据
        distance = GetDistance();

        if (StepMotor_Status == OFF)
        {
            pressure = WS_Read();
            if ((pressure - pressure_init / ReWeighTimes) < 0)
                pressure_delta = (float)(pressure_init / ReWeighTimes - pressure) / 4294.967296f * 100;
            else
                pressure_delta = (float)(pressure - pressure_init / ReWeighTimes) / 4294.967296f * 100;
            // 消除坏数据
            if (pressure_delta > 200000)
                pressure_delta = 0;
            printf("SENSOR:Pressure:%.2f Distance:%.2f\r\n", pressure_delta, distance);
        }
        else
            printf("Distance: %.2f\r\n", distance);

        // 上传至缓冲区
        app_msg = malloc(sizeof(app_msg_t));
        if (app_msg != NULL)
        {
            // printf("read to send messages\n");
            printf("litter:%f\n", pressure_litter);
            printf("shift: %f\n", pressure_shift);
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.LeftoverCatLitter = (float)pressure_litter; // 剩余猫砂
            app_msg->msg.report.Cleaner = (int)Operation_Sign;              // 铲屎信号（1铲   0不铲）
            app_msg->msg.report.CatLitters = (float)pressure_shift;         // 铲掉的排泄物
            if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT) == osOK)
            {
                printf("send out messages, id: %d\n", (int)g_app_cb.app_msg);
                free(app_msg);
            }
        }

        // 界面显示数据
        // printf("ready to show data\n");
        if (1)
        {
            OLED_ShowChinese(42, 0, 2, 12, 1);
            OLED_ShowChinese(56, 0, 3, 12, 1);
            OLED_ShowChinese(70, 0, 22, 12, 1);
        }
        else
        {
            OLED_ShowChinese(42, 0, 4, 12, 1);
            OLED_ShowChinese(56, 0, 14, 12, 1);
            OLED_ShowChinese(70, 0, 13, 12, 1);
        }

        sprintf((char *)str, "%dkg", Shift);
        OLED_ShowString(70, 14, str, 12, 1);

        // 剩余猫砂
        sprintf((char *)str, "%dkg", Litter);
        OLED_ShowString(70, 28, str, 12, 1);

        // 今日铲屎次数
        sprintf((char *)str, "%d", cnt);
        OLED_ShowString(98, 42, str, 12, 1);

        // 数据处理
        // printf("read to deal with data\n");
        if (distance <= Prot_Distance)
        {
            Protec_flag = 1;
            StepMotor_Run(OFF);
            Operation_Sign = 0;
        }
        else
        {
            // 消除误差
            if (pressure_delta - pressure_shift > LitterPressure)
                count++;
            else
                count = 0;

            if (count >= 5)
            {
                count = 0;
                // 电机正转
                StepMotor_Run(FOR);
                StepMotor_Status = FOR;
                Operation_Sign = 1;
                osDelay(100);
                IoTGpioRegisterIsrFunc(14, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LimitedKey_2, NULL);
                IoTGpioRegisterIsrFunc(13, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LimitedKey_1, NULL);

                // 记录数据
                pressure_shift += pressure_delta;
                pressure_litter -= pressure_delta;
                Litter = (int)pressure_litter;
                Shift = (int)pressure_shift;
                cnt++;
            }

            // 恢复工作状态
            if (Protec_flag)
            {
                if (StepMotor_Status != OFF)
                {
                    Operation_Sign = 1;
                    StepMotor_Run(StepMotor_Status);
                }
                Protec_flag = 0;
            }
        }

        hi_watchdog_feed();
        osDelay(50);
    }
}

static void LitterCleaner_Connector(void)
{
    app_msg_t *app_msg;

    while (1)
    {
        app_msg = NULL;
        (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
        printf("connect, id: %d\n", (int)g_app_cb.app_msg);
        if (app_msg != NULL)
        {
            switch (app_msg->msg_type)
            {
            case en_msg_cmd:
                deal_cmd_msg(&app_msg->msg.cmd);
                break;
            case en_msg_report:
                deal_report_msg(&app_msg->msg.report);
                break;
            default:
                break;
            }
            free(app_msg);
        }

        osDelay(100);
    }
}

static void LitterCleaner_Init(void)
{
    char ack = 0;

    // 初始化wifi，如无法初始化，则进入WiFi配置模式
    ack = LitterCleaner_WifiInit();
    if (ack == (-1))
        wifi_start_softap();

    CloudInit();
    WS_Init();
    Hcsr04_Init();
    StepMotor_Init();
    SteeringEngine_Init();
    LitterCleaner_KeyInit();

    // 初始化数据
    for (int i = 0; i < ReWeighTimes; i++)
    {
        pressure_init += WS_Read();
        osDelay(20);
    }
    printf("init successfully\n");

    osThreadAttr_t attr;

    // 按键检测
    attr.name = "Key";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 8192;
    attr.priority = osPriorityAboveNormal2;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Key, NULL, &attr) == NULL)
        printf("[LitterCleaner - Key] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Key] Succeed to creat Task!\n");

    // 数据发送与接收
    attr.name = "Connector";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityAboveNormal;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Connector, NULL, &attr) == NULL)
        printf("[LitterCleaner - Connector] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Connector] Succeed to creat Task!\n");

    // 传感器
    attr.name = "Sensor";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityAboveNormal3;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Sensor, NULL, &attr) == NULL)
        printf("[LitterCleaner - Sensor] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Sensor] Succeed to creat Task!\n");

    osThreadExit();
}

static void LitterCleaner_TaskEntry(void)
{
    osThreadAttr_t attr;

    // 按键检测
    attr.name = "Cleaner_Init";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 8192;
    attr.priority = osPriorityAboveNormal6;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Init, NULL, &attr) == NULL)
        printf("[LitterCleaner - Init] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Init] Succeed to creat Task!\n");
}

SYS_RUN(LitterCleaner_TaskEntry);