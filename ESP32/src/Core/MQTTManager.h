#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "MQTTConfig.h"
#include "../UI/WindChime.h"

#ifdef __cplusplus
extern "C" {
#endif

// MQTT连接状态
typedef enum {
    MQTT_STATUS_DISCONNECTED = 0,
    MQTT_STATUS_CONNECTING,
    MQTT_STATUS_CONNECTED,
    MQTT_STATUS_FAILED,
    MQTT_STATUS_RECONNECTING
} mqtt_status_t;

// 数据源映射
typedef struct {
    const char* source_name;
    data_source_t source_type;
} mqtt_source_mapping_t;

typedef struct {
    int r;
    int g;
    int b;
    float a;
    int x_coord;
    int y_coord;
    int radius;
} circle_style_t;

// MQTT事件数据结构
typedef struct {
    data_source_t source;
    char description_msg[128];
    char description_title[128];
    char time[32];
    char data1[64];
    char data2[64];
    int32_t intensity;
    circle_style_t circle_style;
} mqtt_event_data_t;

// 回调函数类型
typedef void (*mqtt_status_callback_t)(mqtt_status_t status, const char* message);
typedef void (*mqtt_event_callback_t)(const mqtt_event_data_t* event_data);

// MQTT管理器函数
void MQTTManager_Init(void);
void MQTTManager_SetCredentials(const char* username, const char* password);
void MQTTManager_Connect(void);
void MQTTManager_Disconnect(void);
void MQTTManager_Update(void);

// 状态查询
mqtt_status_t MQTTManager_GetStatus(void);
const char* MQTTManager_GetStatusString(void);
bool MQTTManager_IsConnected(void);

// 回调设置
void MQTTManager_SetStatusCallback(mqtt_status_callback_t callback);
void MQTTManager_SetEventCallback(mqtt_event_callback_t callback);

// 订阅管理
bool MQTTManager_Subscribe(const char* topic);
bool MQTTManager_Unsubscribe(const char* topic);

// 发布消息（可选功能）
bool MQTTManager_Publish(const char* topic, const char* payload);

// 工具函数
const char* MQTTManager_GetBrokerInfo(void);
void MQTTManager_PrintStatus(void);

// 状态和心跳发送
void MQTTManager_SendStatusUpdate(void);
void MQTTManager_SendHeartbeat(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_MANAGER_H