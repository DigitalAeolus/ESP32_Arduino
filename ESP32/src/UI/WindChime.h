#ifndef LV_WINDCHIME_H
#define LV_WINDCHIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

// 数据源类型
typedef enum {
    DATA_SOURCE_GITHUB = 0,
    DATA_SOURCE_WIKIPEDIA,
    DATA_SOURCE_WEATHER,
    DATA_SOURCE_MAX
} data_source_t;

// 事件数据结构
typedef struct {
    data_source_t source;
    uint32_t timestamp;
    int32_t intensity;      // 事件强度 (0-100)
    uint32_t color_hash;    // 颜色哈希值
    char description[64];   // 事件描述
} wind_chime_event_t;


extern lv_obj_t * windchime_screen;

// 主要函数
void WindChimeScreenCreate(lv_event_cb_t event_cb);
void WindChimeAddEvent(wind_chime_event_t* event);
void WindChimeUpdateWeather(int16_t wind_speed, int16_t temperature);
void WindChimeSetVolume(uint8_t volume);

// MQTT事件触发函数
void TriggerWindChimeEvent(data_source_t source, int32_t intensity, const char* description);

// 动画和效果函数
void WindChimeStartAnimation(void);
void WindChimeStopAnimation(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_WINDCHIME_H*/