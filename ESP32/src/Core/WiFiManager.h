#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// WiFi状态枚举
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_SCANNING,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_status_t;

// WiFi网络信息结构
typedef struct {
    char ssid[33];
    int32_t rssi;
    wifi_auth_mode_t auth_mode;
    bool is_hidden;
} wifi_network_t;

// WiFi配置结构
typedef struct {
    char ssid[33];
    char password[65];
    bool auto_connect;
} wifi_manager_config_t;

// 回调函数类型
typedef void (*wifi_status_callback_t)(wifi_status_t status, const char* message);
typedef void (*wifi_scan_callback_t)(wifi_network_t* networks, int count);

// WiFi管理器初始化
void WiFiManager_Init(void);

// WiFi扫描
void WiFiManager_StartScan(wifi_scan_callback_t callback);
void WiFiManager_StopScan(void);

// WiFi连接
void WiFiManager_Connect(const char* ssid, const char* password, bool save_config);
void WiFiManager_Disconnect(void);

// 自动连接
void WiFiManager_AutoConnect(void);

// 配置管理
bool WiFiManager_SaveConfig(const char* ssid, const char* password, bool auto_connect);
bool WiFiManager_LoadConfig(wifi_manager_config_t* config);
void WiFiManager_ClearConfig(void);

// 状态查询
wifi_status_t WiFiManager_GetStatus(void);
const char* WiFiManager_GetStatusString(void);
const char* WiFiManager_GetConnectedSSID(void);
int32_t WiFiManager_GetRSSI(void);
IPAddress WiFiManager_GetIP(void);

// 设置状态回调
void WiFiManager_SetStatusCallback(wifi_status_callback_t callback);

// 更新函数（需要在主循环中调用）
void WiFiManager_Update(void);

// 工具函数
const char* WiFiManager_GetAuthModeString(wifi_auth_mode_t auth_mode);
int WiFiManager_GetSignalStrength(int32_t rssi);

// 调试和恢复函数
void WiFiManager_ResetScanState(void);
void WiFiManager_PrintStatus(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H