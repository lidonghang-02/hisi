#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hi_wifi_api.h"
#include "hi_nv.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"

#include "lwip/sockets.h"

#define APP_INIT_VAP_NUM    2
#define APP_INIT_USR_NUM    2
#define WIFI_NVID 0x0B

static struct netif *g_lwip_netif = NULL;

typedef struct {
    hi_u8 ssid[33];
    hi_u8 passwd[65];
}Wifi_InitInfo;

/* clear netif's ip, gateway and netmask */
void hi_sta_reset_addr(struct netif *pst_lwip_netif)
{
    ip4_addr_t st_gw;
    ip4_addr_t st_ipaddr;
    ip4_addr_t st_netmask;
    printf("%s %d \r\n", __FILE__, __LINE__);
    if (pst_lwip_netif == NULL) {
        printf("hisi_reset_addr::Null param of netdev\r\n");
        return;
    }

    IP4_ADDR(&st_gw, 0, 0, 0, 0);
    IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    netifapi_netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);
}

void wifi_wpa_event_cb(const hi_wifi_event *hisi_event)
{
    if (hisi_event == NULL)
        return;

    switch (hisi_event->event) {
        case HI_WIFI_EVT_SCAN_DONE:
            printf("WiFi: Scan results available\n");
            break;
        case HI_WIFI_EVT_CONNECTED:
            printf("WiFi: Connected\n");
            netifapi_dhcp_start(g_lwip_netif);
            break;
        case HI_WIFI_EVT_DISCONNECTED:
            printf("WiFi: Disconnected\n");
            netifapi_dhcp_stop(g_lwip_netif);
            hi_sta_reset_addr(g_lwip_netif);
            break;
        case HI_WIFI_EVT_WPS_TIMEOUT:
            printf("WiFi: wps is timeout\n");
            break;
        default:
            break;
    }
}

int hi_wifi_start_connect(char *ssid, int ssid_len, char *passwd, int passwd_len)
{
    int ret;
    errno_t rc;
    hi_wifi_assoc_request assoc_req = {0};

    /* copy SSID to assoc_req */
    //热点名称
    rc = memcpy_s(assoc_req.ssid, HI_WIFI_MAX_SSID_LEN + 1, ssid, ssid_len); /* 9:ssid length */
    if (rc != EOK) {
        printf("%s %d \r\n", __FILE__, __LINE__);
        return -1;
    }

    /*
     * OPEN mode
     * for WPA2-PSK mode:
     * set assoc_req.auth as HI_WIFI_SECURITY_WPA2PSK,
     * then memcpy(assoc_req.key, "12345678", 8).
     */
    //热点加密方式
    assoc_req.auth = HI_WIFI_SECURITY_WPA2PSK;

    /* 热点密码 */
    memcpy(assoc_req.key, passwd, passwd_len);


    ret = hi_wifi_sta_connect(&assoc_req);
    if (ret != HISI_OK) {
        printf("%s %d \r\n", __FILE__, __LINE__);
        return -1;
    }
    printf("%s %d \r\n", __FILE__, __LINE__);
    return 0;
}

char sta_demo(char *ssid, int ssid_len, char *passwd, int passwd_len)
{
    int ret;
    char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
    int len = sizeof(ifname);

    const unsigned char wifi_vap_res_num = APP_INIT_VAP_NUM;
    const unsigned char wifi_user_res_num = APP_INIT_USR_NUM;

    printf("%s %d \r\n", __FILE__, __LINE__);

    //wifi模块初始化，刚刚我们推出了AP模式。需要重新初始化
    ret = hi_wifi_init(wifi_vap_res_num, wifi_user_res_num);
    if (ret != HISI_OK) {
        printf("%s %d \r\n", __FILE__, __LINE__);
        return -1;
    }

    //启动STA模式
    ret = hi_wifi_sta_start(ifname, &len);
    if (ret != HISI_OK) {
        printf("%s %d \r\n", __FILE__, __LINE__);
        return -1;
    }

    /* 注册wifi事件回调函数，如果成功连接上热点，会有打印信息
     */
    ret = hi_wifi_register_event_callback(wifi_wpa_event_cb);
    if (ret != HISI_OK) {
        printf("register wifi event callback failed\n");
    }

    /* acquire netif for IP operation */
    g_lwip_netif = netifapi_netif_find(ifname);
    if (g_lwip_netif == NULL) {
        printf("%s: get netif failed\n", __FUNCTION__);
        return -1;
    }

    /* 开始进行热点连接 */
    ret = hi_wifi_start_connect(ssid, ssid_len, passwd, passwd_len);
    if (ret != 0) {
        printf("%s %d \r\n", __FILE__, __LINE__);
        return -1;
    }


    return 0;

}


char start_sta_connect(char *ssid, int ssid_len, char *passwd, int passwd_len)
{
    hi_u32 ret = 0;
    Wifi_InitInfo wifi = {0}, nv = {0};
    memset(&wifi, 0, sizeof(Wifi_InitInfo));

    memcpy_s(&wifi.ssid[0], sizeof(Wifi_InitInfo), ssid, HI_WIFI_MAX_SSID_LEN + 1);
    memcpy_s(&wifi.passwd[0], sizeof(Wifi_InitInfo), passwd, HI_WIFI_MAX_SSID_LEN + 1);

    ret = hi_factory_nv_write(WIFI_NVID, &wifi, sizeof(Wifi_InitInfo), 0);
    if (ret != HISI_OK)
        printf("NV write wrong!\n");
    
    ret = hi_factory_nv_read(WIFI_NVID, &nv, sizeof(Wifi_InitInfo), 0);
    if (ret != HISI_OK)
        printf("NV read wrong!\n");
    printf("NV READ: ssid = \"%s\", passwd = \"%s\"\n", nv.ssid, nv.passwd);
    
    return sta_demo(ssid, ssid_len, passwd, passwd_len);
}
