#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"

#include <dtls_al.h>
#include <oc_mqtt_al.h>
#include <mqtt_al.h>
#include "Sensor.h"
#include "wifi_connect.h"
#include "iot_cloud.h"

// 拼装数据上传
static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t LeftoverCatLitter;
    oc_mqtt_profile_kv_t CatLitters;
    oc_mqtt_profile_kv_t Cleaner;

    if (g_app_cb.connected != 1)
    {
        return;
    }

    service.event_time = NULL;
    service.service_id = "cat";
    service.service_property = &LeftoverCatLitter;
    service.nxt = NULL;

    LeftoverCatLitter.key = "LeftoverCatLitter";
    LeftoverCatLitter.value = &report->LeftoverCatLitter;
    LeftoverCatLitter.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    LeftoverCatLitter.nxt = &Cleaner;

    Cleaner.key = "Cleaner";
    Cleaner.value = &report->Cleaner;
    Cleaner.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Cleaner.nxt = &motor;

    CatLitters.key = "CatLitters";
    CatLitters.value = &report->CatLitters;
    CatLitters.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    CatLitters.nxt = NULL;
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
    obj_para = cJSON_GetObjectItem(obj_paras, "BeginClean");
    if (obj_para == NULL)
    {
        cJSON_Delete(obj_root);
    }
    ///< operate the Motor here
    if (strcmp(cJSON_GetStringValue(obj_para), "CLEAN") == 0)
    {
        g_app_cb.motor = 1;
        MotorStatusSet(CLEAN);
        printf("Start Clean!\r\n");
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