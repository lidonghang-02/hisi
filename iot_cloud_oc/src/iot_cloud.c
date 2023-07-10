#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "ohos_init.h"

#include <dtls_al.h>
#include <mqtt_al.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include "Sensor.h"
#include "wifi_connect.h"
#include "iot_cloud.h"

// 拼装数据上传
static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t pressure;
    oc_mqtt_profile_kv_t distance;
    oc_mqtt_profile_kv_t motor;

    if (g_app_cb.connected != 1)
    {
        return;
    }

    service.event_time = NULL;
    service.service_id = "cat";
    service.service_property = &pressure;
    service.nxt = NULL;

    pressure.key = "Pressure";
    pressure.value = &report->pressure;
    pressure.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    pressure.nxt = &distance;

    distance.key = "Distance";
    distance.value = &report->distance;
    distance.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    distance.nxt = &motor;

    motor.key = "MotorStatus";
    if (g_app_cb.motor == FOR)
    {
        motor.value = "FOR";
    }
    else if (g_app_cb.motor == OFF)
    {
        motor.value = "OFF";
    }
    else
    {
        motor.value = "REW";
    }
    motor.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    motor.nxt = NULL;
    // 上传数据
    oc_mqtt_profile_propertyreport(NULL, &service);
    return;
}

// 推送消息至缓冲区
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int ret = 0;
    char *buf;
    int buf_len;
    app_msg_t *app_msg;

    if ((msg == NULL) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS))
    {
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if (buf == NULL)
    {
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy_s(app_msg->msg.cmd.request_id, buf_len, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy_s(app_msg->msg.cmd.payload, buf_len, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT);
    if (ret != 0)
    {
        free(app_msg);
    }

    return ret;
}

static void oc_cmdresp(cmd_t *cmd, int cmdret)
{
    oc_mqtt_profile_cmdresp_t cmdresp;
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_motor_cmd(cmd_t *cmd, cJSON *obj_root)
{
    cJSON *obj_paras;
    cJSON *obj_para;
    int cmdret;

    obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
    if (obj_paras == NULL)
    {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "Motor");
    if (obj_para == NULL)
    {
        cJSON_Delete(obj_root);
    }
    ///< operate the Motor here
    if (strcmp(cJSON_GetStringValue(obj_para), "FOR") == 0)
    {
        g_app_cb.motor = 1;
        MotorStatusSet(FOR);
        printf("Motor FOR!\r\n");
    }
    else if (strcmp(cJSON_GetStringValue(obj_para), "REW") == 1)
    {
        g_app_cb.motor = 1;
        MotorStatusSet(REW);
        printf("Motor REW!\r\n");
    }
    else
    {
        g_app_cb.motor = 0;
        MotorStatusSet(OFF);
        printf("Motor Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}

static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;

    int cmdret = 1;
    obj_root = cJSON_Parse(cmd->payload);
    if (obj_root == NULL)
    {
        oc_cmdresp(cmd, cmdret);
    }
    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (obj_cmdname == NULL)
    {
        cJSON_Delete(obj_root);
    }
    if (strcmp(cJSON_GetStringValue(obj_cmdname), "Change_Motor_Status") == 0)
    {
        deal_motor_cmd(cmd, obj_root);
    }

    return;
}
// 连接平台
void CloudInit(void)
{
    app_msg_t *app_msg;
    uint32_t ret;

    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (g_app_cb.app_msg == NULL)
    {
        printf("Create receive msg queue failed");
    }
    oc_mqtt_profile_connect_t connect_para;
    (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));

    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if ((ret == (int)en_oc_mqtt_err_ok))
    {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    }
    else
    {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }
}
// void CloudMainTaskEntry(void)
// {
//     app_msg_t *app_msg;
//     uint32_t ret;

//     WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
//     dtls_al_init();
//     mqtt_al_init();
//     oc_mqtt_init();

//     g_app_cb.app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
//     if (g_app_cb.app_msg == NULL)
//     {
//         printf("Create receive msg queue failed");
//     }
//     oc_mqtt_profile_connect_t connect_para;
//     (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));

//     connect_para.boostrap = 0;
//     connect_para.device_id = CONFIG_APP_DEVICEID;
//     connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
//     connect_para.server_addr = CONFIG_APP_SERVERIP;
//     connect_para.server_port = CONFIG_APP_SERVERPORT;
//     connect_para.life_time = CONFIG_APP_LIFETIME;
//     connect_para.rcvfunc = msg_rcv_callback;
//     connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
//     ret = oc_mqtt_profile_connect(&connect_para);
//     if ((ret == (int)en_oc_mqtt_err_ok))
//     {
//         g_app_cb.connected = 1;
//         printf("oc_mqtt_profile_connect succed!\r\n");
//     }
//     else
//     {
//         printf("oc_mqtt_profile_connect faild!\r\n");
//     }

//     while (1)
//     {
//         app_msg = NULL;
//         (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
//         if (app_msg != NULL)
//         {
//             switch (app_msg->msg_type)
//             {
//             case en_msg_cmd:
//                 deal_cmd_msg(&app_msg->msg.cmd);
//                 break;
//             case en_msg_report:
//                 deal_report_msg(&app_msg->msg.report);
//                 break;
//             default:
//                 break;
//             }
//             free(app_msg);
//         }
//     }
// }

// void SensorTaskEntry(void)
// {
//     app_msg_t *app_msg;
//     int ret;
//     SensorData data;
//     SensorIoInit();
//     while (1)
//     {
//         ret = SensorReadData(&data);

//         if (ret != 0)
//         {
//             printf("Sensor Read Data failed!\r\n");
//             return;
//         }
//         app_msg = malloc(sizeof(app_msg_t));
//         printf("SENSOR:Pressure:%.2f Distance:%.2f\r\n", data.Pressure, data.Distance);
//         if (app_msg != NULL)
//         {
//             app_msg->msg_type = en_msg_report;
//             app_msg->msg.report.pressure = (int)data.Pressure;
//             app_msg->msg.report.distance = (int)data.Distance;
//             if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT != 0))
//             {
//                 free(app_msg);
//             }
//         }
//         sleep(TASK_DELAY);
//     }
// }

// static void IotMainTaskEntry(void)
// {
//     osThreadAttr_t attr;

//     attr.name = "CloudMainTaskEntry";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = CLOUD_TASK_STACK_SIZE;
//     attr.priority = CLOUD_TASK_PRIO;

//     if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL)
//     {
//         printf("Failed to create CloudMainTaskEntry!\n");
//     }
//     attr.stack_size = SENSOR_TASK_STACK_SIZE;
//     attr.priority = SENSOR_TASK_PRIO;
//     attr.name = "SensorTaskEntry";
//     if (osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr) == NULL)
//     {
//         printf("Failed to create SensorTaskEntry!\n");
//     }
// }

// APP_FEATURE_INIT(IotMainTaskEntry);