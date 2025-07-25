#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// WiFi配网界面
extern lv_obj_t * wifi_config_screen;

// 创建WiFi配网界面
void WiFiConfigCreate(lv_event_cb_t event_cb);

// 显示WiFi配网界面
void WiFiConfigShow(void);

// 隐藏WiFi配网界面
void WiFiConfigHide(void);

// 更新WiFi状态显示
void WiFiConfigUpdateStatus(const char* status, bool is_error);

// 更新网络列表
void WiFiConfigUpdateNetworks(void);

// 刷新当前连接信息
void WiFiConfigRefreshConnection(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_CONFIG_H