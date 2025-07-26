#include "MQTTManager.h"
#include "WiFiManager.h"
#include "../UI/WindChime.h"
#include "../UI/AudioFeedback.h"
#include "../UI/DataSimulator.h"
#include <string.h>

// 静态变量
static WiFiClient wifi_client;
static PubSubClient mqtt_client(wifi_client);
static mqtt_status_t current_status = MQTT_STATUS_DISCONNECTED;
static mqtt_status_callback_t status_callback = NULL;
static mqtt_event_callback_t event_callback = NULL;
static char mqtt_username[64] = {0};
static char mqtt_password[64] = {0};
static unsigned long last_connect_attempt = 0;
static unsigned long last_heartbeat = 0;
static const unsigned long RECONNECT_INTERVAL = MQTT_RECONNECT_INTERVAL;
static const unsigned long HEARTBEAT_INTERVAL = MQTT_HEARTBEAT_INTERVAL;

// 数据源映射表
static const mqtt_source_mapping_t source_mappings[] = {
    {"github", DATA_SOURCE_GITHUB},
    {"wiki", DATA_SOURCE_WIKIPEDIA},
    {"wikipedia", DATA_SOURCE_WIKIPEDIA},
    {"weather", DATA_SOURCE_WEATHER},
    {"wind", DATA_SOURCE_WEATHER}
};
static const int source_mappings_count = sizeof(source_mappings) / sizeof(source_mappings[0]);

// 内部函数声明
static void update_status(mqtt_status_t status, const char* message);
static void mqtt_callback(char* topic, byte* payload, unsigned int length);
static void process_mqtt_message(const char* topic, const char* payload);
static data_source_t map_source_string(const char* source_str);
static int32_t calculate_intensity(const mqtt_event_data_t* event_data);
static bool reconnect_mqtt(void);
static void send_heartbeat(void);
static void send_status_update(void);
static void send_device_info(void);

void MQTTManager_Init(void)
{
    Serial.println("MQTTManager: Initializing...");
    
    // 设置MQTT服务器和回调
    mqtt_client.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqtt_client.setCallback(mqtt_callback);
    mqtt_client.setBufferSize(MQTT_BUFFER_SIZE);
    mqtt_client.setKeepAlive(MQTT_KEEPALIVE_INTERVAL);
    
    current_status = MQTT_STATUS_DISCONNECTED;
    
    Serial.printf("MQTTManager: Configured for broker %s:%d\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    Serial.println("MQTTManager: Initialized");
}

void MQTTManager_SetCredentials(const char* username, const char* password)
{
    if (username) {
        strncpy(mqtt_username, username, sizeof(mqtt_username) - 1);
        mqtt_username[sizeof(mqtt_username) - 1] = '\0';
    }
    
    if (password) {
        strncpy(mqtt_password, password, sizeof(mqtt_password) - 1);
        mqtt_password[sizeof(mqtt_password) - 1] = '\0';
    }
    
    Serial.printf("MQTTManager: Credentials set for user: %s\n", username ? username : "anonymous");
}

void MQTTManager_Connect(void)
{
    // 检查WiFi连接
    if (WiFiManager_GetStatus() != WIFI_STATUS_CONNECTED) {
        update_status(MQTT_STATUS_FAILED, "WiFi not connected");
        return;
    }
    
    if (current_status == MQTT_STATUS_CONNECTING || current_status == MQTT_STATUS_CONNECTED) {
        return; // 已经在连接或已连接
    }
    
    Serial.println("MQTTManager: Starting connection...");
    update_status(MQTT_STATUS_CONNECTING, "Connecting to MQTT broker...");
    last_connect_attempt = millis();
}

void MQTTManager_Disconnect(void)
{
    if (mqtt_client.connected()) {
        mqtt_client.disconnect();
    }
    update_status(MQTT_STATUS_DISCONNECTED, "Disconnected from MQTT broker");
    Serial.println("MQTTManager: Disconnected");
}

void MQTTManager_Update(void)
{
    // 处理MQTT客户端循环
    if (mqtt_client.connected()) {
        mqtt_client.loop();
        
        // 发送心跳
        if (millis() - last_heartbeat > HEARTBEAT_INTERVAL) {
            send_heartbeat();
            last_heartbeat = millis();
        }
    } else {
        // 如果应该连接但未连接，尝试重连
        if (current_status == MQTT_STATUS_CONNECTING || current_status == MQTT_STATUS_RECONNECTING) {
            if (millis() - last_connect_attempt > RECONNECT_INTERVAL) {
                reconnect_mqtt();
            }
        }
    }
    
    // 检查WiFi状态变化
    static wifi_status_t last_wifi_status = WIFI_STATUS_DISCONNECTED;
    wifi_status_t current_wifi_status = WiFiManager_GetStatus();
    
    if (last_wifi_status != current_wifi_status) {
        last_wifi_status = current_wifi_status;
        
        if (current_wifi_status != WIFI_STATUS_CONNECTED && mqtt_client.connected()) {
            update_status(MQTT_STATUS_FAILED, "WiFi connection lost");
        } else if (current_wifi_status == WIFI_STATUS_CONNECTED && 
                   current_status == MQTT_STATUS_FAILED) {
            // WiFi重新连接，尝试重连MQTT
            update_status(MQTT_STATUS_RECONNECTING, "WiFi restored, reconnecting...");
            last_connect_attempt = millis() - RECONNECT_INTERVAL; // 立即尝试重连
        }
    }
}

mqtt_status_t MQTTManager_GetStatus(void)
{
    return current_status;
}

const char* MQTTManager_GetStatusString(void)
{
    switch (current_status) {
        case MQTT_STATUS_DISCONNECTED: return "Disconnected";
        case MQTT_STATUS_CONNECTING: return "Connecting";
        case MQTT_STATUS_CONNECTED: return "Connected";
        case MQTT_STATUS_FAILED: return "Failed";
        case MQTT_STATUS_RECONNECTING: return "Reconnecting";
        default: return "Unknown";
    }
}

bool MQTTManager_IsConnected(void)
{
    return current_status == MQTT_STATUS_CONNECTED && mqtt_client.connected();
}

void MQTTManager_SetStatusCallback(mqtt_status_callback_t callback)
{
    status_callback = callback;
}

void MQTTManager_SetEventCallback(mqtt_event_callback_t callback)
{
    event_callback = callback;
}

bool MQTTManager_Subscribe(const char* topic)
{
    if (!mqtt_client.connected()) {
        Serial.printf("MQTTManager: Cannot subscribe to %s - not connected\n", topic);
        return false;
    }
    
    bool result = mqtt_client.subscribe(topic);
    Serial.printf("MQTTManager: Subscribe to %s: %s\n", topic, result ? "Success" : "Failed");
    return result;
}

bool MQTTManager_Unsubscribe(const char* topic)
{
    if (!mqtt_client.connected()) {
        return false;
    }
    
    bool result = mqtt_client.unsubscribe(topic);
    Serial.printf("MQTTManager: Unsubscribe from %s: %s\n", topic, result ? "Success" : "Failed");
    return result;
}

bool MQTTManager_Publish(const char* topic, const char* payload)
{
    if (!mqtt_client.connected()) {
        Serial.printf("MQTTManager: Cannot publish to %s - not connected\n", topic);
        return false;
    }
    
    bool result = mqtt_client.publish(topic, payload);
    Serial.printf("MQTTManager: Publish to %s: %s\n", topic, result ? "Success" : "Failed");
    return result;
}

const char* MQTTManager_GetBrokerInfo(void)
{
    static char broker_info[128];
    snprintf(broker_info, sizeof(broker_info), "%s:%d", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    return broker_info;
}

void MQTTManager_PrintStatus(void)
{
    Serial.printf("MQTTManager Status:\n");
    Serial.printf("  Status: %s\n", MQTTManager_GetStatusString());
    Serial.printf("  Broker: %s\n", MQTTManager_GetBrokerInfo());
    Serial.printf("  Client Connected: %s\n", mqtt_client.connected() ? "Yes" : "No");
    Serial.printf("  WiFi Status: %s\n", WiFiManager_GetStatusString());
    if (mqtt_client.connected()) {
        Serial.printf("  Client State: %d\n", mqtt_client.state());
    }
}

void MQTTManager_SendStatusUpdate(void)
{
    send_status_update();
}

void MQTTManager_SendHeartbeat(void)
{
    send_heartbeat();
}

// 内部函数实现
static void update_status(mqtt_status_t status, const char* message)
{
    current_status = status;
    Serial.printf("MQTTManager: Status changed to %s - %s\n", 
                  MQTTManager_GetStatusString(), message ? message : "");
    
    if (status_callback) {
        status_callback(status, message);
    }
}

static void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
    // 创建以null结尾的字符串
    char* message = (char*)malloc(length + 1);
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("MQTTManager: Received message on topic %s: %s\n", topic, message);
    
    // 处理消息
    process_mqtt_message(topic, message);
    
    free(message);
}

static void process_mqtt_message(const char* topic, const char* payload)
{
    // 解析JSON消息
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf("MQTTManager: JSON parsing failed: %s\n", error.c_str());
        return;
    }
    
    // 检查是否有data字段
    if (!doc["data"].is<JsonObject>()) {
        Serial.println("MQTTManager: No 'data' field in message");
        return;
    }
    
    JsonObject data = doc["data"];
    
    // 创建事件数据结构
    mqtt_event_data_t event_data;
    memset(&event_data, 0, sizeof(event_data));
    
    // 解析source字段
    const char* source_str = data["source"] | "unknown";
    event_data.source = map_source_string(source_str);
    
    // 解析其他字段
    strncpy(event_data.description_msg, data["description_msg"] | "", sizeof(event_data.description_msg) - 1);
    strncpy(event_data.description_title, data["description_title"] | "", sizeof(event_data.description_title) - 1);
    strncpy(event_data.time, data["time"] | "", sizeof(event_data.time) - 1);
    strncpy(event_data.data1, data["data1"] | "", sizeof(event_data.data1) - 1);
    strncpy(event_data.data2, data["data2"] | "", sizeof(event_data.data2) - 1);

    event_data.circle_style.x_coord = data["style"]["x_coord"] | 200;
    event_data.circle_style.y_coord = data["style"]["y_coord"] | 200;
    event_data.circle_style.radius = data["style"]["radius"] | 50;
    event_data.circle_style.r = data["style"]["color"]["r"] | 255;
    event_data.circle_style.g = data["style"]["color"]["g"] | 255;
    event_data.circle_style.b = data["style"]["color"]["b"] | 255;
    event_data.circle_style.a = data["style"]["color"]["a"] | 1.0;

    // 计算强度
    event_data.intensity = calculate_intensity(&event_data);
    
    Serial.printf("MQTTManager: Processed event - Source: %d, Intensity: %d\n", 
                  event_data.source, event_data.intensity);
    
    // 调用事件回调
    if (event_callback) {
        event_callback(&event_data);
    }
    
    // 直接触发风铃事件
    TriggerWindChimeEvent(event_data.source, event_data.intensity, event_data.description_title, &event_data.circle_style);
}

static data_source_t map_source_string(const char* source_str)
{
    if (!source_str) return DATA_SOURCE_GITHUB; // 默认值
    
    for (int i = 0; i < source_mappings_count; i++) {
        if (strcasecmp(source_str, source_mappings[i].source_name) == 0) {
            return source_mappings[i].source_type;
        }
    }
    
    // 如果没有匹配，根据字符串内容猜测
    if (strstr(source_str, "git") != NULL) {
        return DATA_SOURCE_GITHUB;
    } else if (strstr(source_str, "wiki") != NULL) {
        return DATA_SOURCE_WIKIPEDIA;
    } else if (strstr(source_str, "weather") != NULL || strstr(source_str, "wind") != NULL) {
        return DATA_SOURCE_WEATHER;
    }
    
    return DATA_SOURCE_GITHUB; // 默认值
}

static int32_t calculate_intensity(const mqtt_event_data_t* event_data)
{
    int32_t intensity = 50; // 基础强度
    
    // 根据描述长度调整强度
    int desc_len = strlen(event_data->description_msg) + strlen(event_data->description_title);
    intensity += (desc_len / 10); // 每10个字符增加1点强度
    
    // 根据数据源调整强度
    switch (event_data->source) {
        case DATA_SOURCE_GITHUB:
            intensity += 20; // GitHub事件通常比较重要
            break;
        case DATA_SOURCE_WIKIPEDIA:
            intensity += 10; // Wikipedia编辑中等重要
            break;
        case DATA_SOURCE_WEATHER:
            intensity += 30; // 天气事件影响较大
            break;
    }
    
    // 限制强度范围
    if (intensity > 100) intensity = 100;
    if (intensity < 10) intensity = 10;
    
    return intensity;
}

static bool reconnect_mqtt(void)
{
    last_connect_attempt = millis();
    
    // 检查WiFi连接
    if (WiFiManager_GetStatus() != WIFI_STATUS_CONNECTED) {
        update_status(MQTT_STATUS_FAILED, "WiFi not connected");
        return false;
    }
    
    Serial.printf("MQTTManager: Attempting to connect to %s:%d\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    
    // 生成唯一的客户端ID
    char client_id[64];
    snprintf(client_id, sizeof(client_id), "%s%08X", MQTT_CLIENT_ID_PREFIX, (uint32_t)ESP.getEfuseMac());
    
    // 尝试连接
    bool connected = false;
    if (strlen(mqtt_username) > 0) {
        connected = mqtt_client.connect(client_id, mqtt_username, mqtt_password);
    } else {
        connected = mqtt_client.connect(client_id);
    }
    
    if (connected) {
        Serial.println("MQTTManager: Connected to MQTT broker");
        update_status(MQTT_STATUS_CONNECTED, "Connected to MQTT broker");
        
        // 订阅主题
        if (MQTTManager_Subscribe(MQTT_TOPIC_EVENTS)) {
            Serial.printf("MQTTManager: Subscribed to topic: %s\n", MQTT_TOPIC_EVENTS);
        } else {
            Serial.printf("MQTTManager: Failed to subscribe to topic: %s\n", MQTT_TOPIC_EVENTS);
        }
        
        // 发送设备信息和初始状态
        send_device_info();
        send_status_update();
        
        // 重置心跳计时器
        last_heartbeat = millis();
        
        return true;
    } else {
        Serial.printf("MQTTManager: Connection failed, state: %d\n", mqtt_client.state());
        update_status(MQTT_STATUS_RECONNECTING, "Connection failed, retrying...");
        return false;
    }
}

// 发送心跳消息
static void send_heartbeat(void)
{
    if (!mqtt_client.connected()) {
        return;
    }
    
    JsonDocument heartbeat_doc;
    heartbeat_doc["device_id"] = WiFi.macAddress();
    heartbeat_doc["timestamp"] = millis();
    heartbeat_doc["uptime"] = millis() / 1000;
    heartbeat_doc["free_heap"] = ESP.getFreeHeap();
    heartbeat_doc["wifi_rssi"] = WiFi.RSSI();
    heartbeat_doc["wifi_ssid"] = WiFi.SSID();
    
    String heartbeat_payload;
    serializeJson(heartbeat_doc, heartbeat_payload);
    
    if (MQTTManager_Publish(MQTT_TOPIC_HEARTBEAT, heartbeat_payload.c_str())) {
        Serial.println("MQTTManager: Heartbeat sent");
    } else {
        Serial.println("MQTTManager: Failed to send heartbeat");
    }
}

// 发送状态更新
static void send_status_update(void)
{
    if (!mqtt_client.connected()) {
        return;
    }
    
    JsonDocument status_doc;
    status_doc["device_id"] = WiFi.macAddress();
    status_doc["timestamp"] = millis();
    status_doc["mqtt_status"] = MQTTManager_GetStatusString();
    status_doc["wifi_status"] = WiFiManager_GetStatusString();
    status_doc["wifi_ssid"] = WiFi.SSID();
    status_doc["ip_address"] = WiFi.localIP().toString();
    status_doc["mac_address"] = WiFi.macAddress();
    status_doc["free_heap"] = ESP.getFreeHeap();
    status_doc["uptime"] = millis() / 1000;
    status_doc["audio_volume"] = GetAudioVolume();
    status_doc["simulator_running"] = DataSimulatorIsRunning();
    
    String status_payload;
    serializeJson(status_doc, status_payload);
    
    if (MQTTManager_Publish(MQTT_TOPIC_STATUS, status_payload.c_str())) {
        Serial.println("MQTTManager: Status update sent");
    } else {
        Serial.println("MQTTManager: Failed to send status update");
    }
}

// 发送设备信息（连接时发送一次）
static void send_device_info(void)
{
    if (!mqtt_client.connected()) {
        return;
    }
    
    JsonDocument device_doc;
    device_doc["device_id"] = WiFi.macAddress();
    device_doc["device_type"] = "WindChime_ESP32";
    device_doc["firmware_version"] = "1.0.0";
    device_doc["hardware_version"] = "ESP32-S3";
    device_doc["chip_model"] = ESP.getChipModel();
    device_doc["chip_revision"] = ESP.getChipRevision();
    device_doc["cpu_freq"] = ESP.getCpuFreqMHz();
    device_doc["flash_size"] = ESP.getFlashChipSize();
    device_doc["psram_size"] = ESP.getPsramSize();
    device_doc["mac_address"] = WiFi.macAddress();
    device_doc["wifi_ssid"] = WiFi.SSID();
    device_doc["ip_address"] = WiFi.localIP().toString();
    device_doc["mqtt_broker"] = MQTT_BROKER_HOST;
    device_doc["mqtt_port"] = MQTT_BROKER_PORT;
    device_doc["subscribed_topics"] = MQTT_TOPIC_EVENTS;
    device_doc["timestamp"] = millis();
    
    String device_payload;
    serializeJson(device_doc, device_payload);
    
    char device_info_topic[128];
    snprintf(device_info_topic, sizeof(device_info_topic), "windchime/device/%s/info", WiFi.macAddress().c_str());
    
    if (MQTTManager_Publish(device_info_topic, device_payload.c_str())) {
        Serial.println("MQTTManager: Device info sent");
    } else {
        Serial.println("MQTTManager: Failed to send device info");
    }
}