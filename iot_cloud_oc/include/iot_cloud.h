#ifndef __IOT_CLOUD_H__
#define __IOT_CLOUD_H__

#define CONFIG_WIFI_SSID "test"            // 修改为自己的WiFi 热点账号
#define CONFIG_WIFI_PWD "5m49c66m"         // 修改为自己的WiFi 热点密码
#define CONFIG_APP_SERVERIP "117.78.5.125" // 标准版
#define CONFIG_APP_SERVERPORT "1883"
#define CONFIG_APP_DEVICEID "64a40e71ae80ef457fc02cc0_20230705" // 替换为注册设备后生成的deviceid
#define CONFIG_APP_DEVICEPWD "20230705"                         // 替换为注册设备后生成的密钥
#define CONFIG_APP_LIFETIME 60                                  // seconds
#define CONFIG_QUEUE_TIMEOUT (5 * 1000)

#define MSGQUEUE_COUNT 16
#define MSGQUEUE_SIZE 10
#define CLOUD_TASK_STACK_SIZE (1024 * 10)
#define CLOUD_TASK_PRIO 24
#define SENSOR_TASK_STACK_SIZE (1024 * 4)
#define SENSOR_TASK_PRIO 25
#define TASK_DELAY 3

#include "cJSON.h"
#include "oc_mqtt_profile.h"
#include "cmsis_os2.h"

typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
    en_msg_conn,
    en_msg_disconn,
} en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct
{
    float LeftoverCatLitter; // 剩余猫砂
    int Cleaner;             // 铲屎信号
    float CatLitters;        // 铲掉的排泄物
} SensorData, report_t;

typedef struct
{
    en_msg_type_t msg_type;
    union
    {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct
{
    osMessageQueueId_t app_msg;
    int connected;
    int motor;
} app_cb_t;
static app_cb_t g_app_cb;

// 拼装数据上传
void deal_report_msg(report_t *report);

// 推送消息至缓冲区
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg);

static void oc_cmdresp(cmd_t *cmd, int cmdret);

void deal_motor_cmd(cmd_t *cmd, cJSON *obj_root);

void deal_cmd_msg(cmd_t *cmd);
// 连接平台
void CloudInit(void);

void CloudMainTaskEntry(void);
// 传感器
void SensorTaskEntry(void);

#endif
