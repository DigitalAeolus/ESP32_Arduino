#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// MQTT服务器配置
#define MQTT_BROKER_HOST "broker.emqx.io"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID_PREFIX "WindChime_"
#define MQTT_KEEPALIVE_INTERVAL 60
#define MQTT_BUFFER_SIZE 1024

// MQTT主题配置
#define MQTT_TOPIC_EVENTS "windchime/events"
#define MQTT_TOPIC_STATUS "windchime/status"
#define MQTT_TOPIC_HEARTBEAT "windchime/heartbeat"

// 连接配置
#define MQTT_RECONNECT_INTERVAL 5000    // 5秒重连间隔
#define MQTT_CONNECT_TIMEOUT 10000      // 10秒连接超时
#define MQTT_HEARTBEAT_INTERVAL 30000   // 30秒心跳间隔

// QoS配置
#define MQTT_QOS_EVENTS 1               // 事件消息QoS
#define MQTT_QOS_STATUS 0               // 状态消息QoS
#define MQTT_QOS_HEARTBEAT 0            // 心跳消息QoS

// 消息配置
#define MQTT_MAX_MESSAGE_SIZE 512       // 最大消息大小
#define MQTT_MAX_TOPIC_LENGTH 64        // 最大主题长度

#endif // MQTT_CONFIG_H