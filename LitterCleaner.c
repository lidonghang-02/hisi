#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "iot_gpio.h"
// #include "iot_gpio_ex.h"
#include "hi_wifi_api.h"
#include "hi_nv.h"

#include "wifi_config/ap_mode.h"
#include "wifi_config/sta_mode.h"
#include "EasyWifi/wifi_connecter.h"

#include "Weight_Sensor/Weight_Sensor.h"
#include "StepMotor/StepMotor.h"
#include "SteeringEngine/SteeringEngine.h"
#include "HCSR04.h"
#include "iot_cloud.h"

#define KEYLONGTIME 150
#define KEYCLICKTIME 10
#define Key_MaxClick 3

#define WIFI_NVID 0x0B

// 保护距离
#define Prot_Distance 10

// 重复测量次数
#define ReWeighTimes 20

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

struct LitterCleaner_Key
{
    KeyStatus key_status;
    uint32_t key_last;
    uint16_t key_click;
} Key = {1, 0, 0};

// 按键状态机的软件定时器
osTimerId_t KeyTimer_ID = 0;

// 限位开关定时器
osTimerId_t StepMotor_KeyID = 0;
// 防夹猫标志位
uint8_t Protec_flag = 0;
// 电机运转标识
int StepMotor_Status = 0;

float distance = 0;              // 距离
float pressure = 0;              // 变化重量
float pressure_litter = 0;       // 猫砂重量
unsigned long pressure_init = 0; // 初始化重量（猫砂重量的过渡变量）

float distance = 0;              // 距离
float pressure = 0;              // 变化重量
float pressure_litter = 0;       // 猫砂重量
unsigned long pressure_init = 0; // 初始化重量（猫砂重量的过渡变量）

void LitterCleaner_KeyCallback(void);
void LitterCleaner_KeyRegconize(void);
static void LitterCleaner_TaskEntry(void);
static void LitterCleaner_Key(void);
static void LitterCleaner_KeyInit(void);
static char LitterCleaner_WifiInit(void);

/**
 * @brief 按键外部中断回调函数
 *
 */
void LitterCleaner_KeyCallback(void)
{
    Key.key_status = Key_Down;

    osTimerStart(KeyTimer_ID, 1);

    // printf("Get into the key interrupt function\n");
}

/**
 * @brief 按键中断状态机
 *
 */
void LitterCleaner_KeyRegconize(void)
{
    static uint16_t wait_time = 0;

    IoTGpioGetInputVal(7, &Key.key_status);

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
            osTimerStop(KeyTimer_ID);
        }
        Key.key_last = 0;
    }
    else
    {
        wait_time = 0;
        Key.key_click = 0;
        Key.key_last = 0;
        osTimerStop(KeyTimer_ID);
    }

    // printf("Get into the key timer\n");
}

/**
 * @brief 限位开关处理
 *
 */
static void StepMotor_LimitedKey(void)
{
}

/**
 * @brief 按键初始化，采用外部中断方式
 *
 */
static void LitterCleaner_KeyInit(void)
{
    // 按键初始化
    IoTGpioInit(7);
    IoTGpioSetFunc(7, IOT_GPIO_FUNC_GPIO_7_GPIO);
    IoTGpioSetDir(7, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(7, IOT_GPIO_PULL_UP);
    IoTGpioSetOutputVal(7, IOT_GPIO_VALUE1);

    KeyTimer_ID = osTimerNew(LitterCleaner_KeyRegconize, osTimerPeriodic, NULL, NULL);
    if (KeyTimer_ID == NULL)
        printf("KeyTimer Init Fail!\n");

    IoTGpioRegisterIsrFunc(7, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, (GpioIsrCallbackFunc)LitterCleaner_KeyCallback, NULL);

    // 限位开关初始化
}

/**
 * @brief wifi初始化连接
 *
 */
static char LitterCleaner_WifiInit(void)
{
    WifiDeviceConfig Wifi_Config = {0};
    Wifi_InitInfo wifi = {0};
    hi_u32 ret = 0;

    printf("Ready to connect to wifi...\n");

    // ret = hi_factory_nv_read(WIFI_NVID, &wifi, sizeof(Wifi_InitInfo), 0);
    // if (ret == HISI_OK)
    //     printf("Succeed getting the wifi info\n");
    // else
    //     return -1;

    // test init
    strcpy_s(wifi.ssid, WIFI_MAX_SSID_LEN, "MSI");
    strcpy_s(wifi.passwd, WIFI_MAX_KEY_LEN, "128215781");

    printf("ssid: %s , passwd: %s \n", wifi.ssid, wifi.passwd);

    strcpy_s(Wifi_Config.ssid, WIFI_MAX_SSID_LEN, wifi.ssid);
    strcpy_s(Wifi_Config.preSharedKey, WIFI_MAX_KEY_LEN, wifi.passwd);
    Wifi_Config.securityType = WIFI_SEC_TYPE_PSK;

    printf("Finish setting wifi_config\n");

    int Wifi_netID = ConnectToHotspot(&Wifi_Config);
    if (Wifi_netID)
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
        if (Key.key_status == Key_Down)
        {
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
            }
            else if (i == 2) // 双击
            {
                printf("Key Double\n");
                for (int i = 0; i < ReWeighTimes; i++)
                {
                    pressure_init += WS_Read();
                    osDelay(10);
                }
                pressure_litter = (float)pressure_init / 4294.967296f / ReWeighTimes * 100;
            }
            else // 单击
            {
                printf("Key Down\n");
            }
        }
        osDelay(10);
    }
}

static void LitterCleaner_Protector(void)
{
    while (1)
    {
        if (distance <= Prot_Distance)
        {
            Protec_flag = 1;
            StepMotor_Run(OFF);
        }
        osDelay(10);
    }
}

static void LitterCleaner_Sensor(void)
{
    // 已铲猫屎
    int Litter = 0;
    int Shift = 0;
    uint8_t str[64];
    int cnt = 0; // 次数

    app_msg_t *app_msg;
    while (1)
    {
        // 读取数据
        distance = GetDistance();
        pressure = (float)(WS_Read() - pressure_init / ReWeighTimes) / 4294.967296f * 100;
        if (pressure < 0)
            pressure -= pressure;

        // printf("SENSOR:Pressure:%.2f Distance:%.2f\r\n", pressure, distance);

        // 上传至缓冲区
        app_msg = malloc(sizeof(app_msg_t));
        if (app_msg != NULL)
        {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.LeftoverCatLitter = (float)pressure_litter; // 剩余猫砂
            app_msg->msg.report.Cleaner = (int)StepMotor_Status;            // 铲屎信号（1铲   0不铲）
            app_msg->msg.report.LeftoverCatLitter = (float)Shift;           // 铲掉的排泄物
            if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT != 0))
            {
                free(app_msg);
            }
        }
        // 界面显示数据

        // 判断状态（停机/运行中）
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

        sprintf((char *)str, "%dkg", Shift);
        OLED_ShowString(70, 14, str, 12, 1);

        // 剩余猫砂
        sprintf((char *)str, "%dkg", pressure_litter);
        OLED_ShowString(70, 28, str, 12, 1);

        // 今日铲屎次数
        sprintf((char *)str, "%d", cnt);
        OLED_ShowString(98, 42, str, 12, 1);

        // 数据处理
        if (!Protec_flag)
        {
            if (pressure > LitterPressure && distance > Prot_Distance)
            {
                // 初始化限位开关
                StepMotor_KeyID = osTimerNew(StepMotor_LimitedKey, osTimerPeriodic, NULL, NULL);
                osTimerStart(StepMotor_KeyID, 1);

                // 电机正转
                StepMotor_Run(FOR);

                // 记录数据
                Shift += pressure;
                pressure_litter -= pressure;
                Litter = (int)pressure_litter;
                cnt++;
            }
        }

        osDelay(10);
    }
}

static void LitterCleaner_Connector(void)
{
    app_msg_t *app_msg;

    while (1)
    {
        app_msg = NULL;
        (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
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
    LitterCleaner_KeyInit();
    StepMotor_Init();
    SteeringEngine_Init();

    osThreadAttr_t attr;

    // 按键检测
    attr.name = "Key";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 8192;
    attr.priority = osPriorityAboveNormal1;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Key, NULL, &attr) == NULL)
        printf("[LitterCleaner - Key] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Key] Succeed to creat Task!\n");

    // 防夹猫
    attr.name = "Protector";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 8192;
    attr.priority = osPriorityAboveNormal6;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Key, NULL, &attr) == NULL)
        printf("[LitterCleaner - Protector] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Protector] Succeed to creat Task!\n");

    // 传感器
    attr.name = "Sensor";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityAboveNormal;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Sensor, NULL, &attr) == NULL)
        printf("[LitterCleaner - Sensor] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Sensor] Succeed to creat Task!\n");

    // 数据发送与接收
    attr.name = "Connector";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityAboveNormal;

    if (osThreadNew((osThreadFunc_t)LitterCleaner_Sensor, NULL, &attr) == NULL)
        printf("[LitterCleaner - Connector] Falied to create Task!\n");
    else
        printf("[LitterCleaner - Connector] Succeed to creat Task!\n");

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