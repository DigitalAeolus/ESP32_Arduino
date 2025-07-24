#ifndef AUDIO_FEEDBACK_H
#define AUDIO_FEEDBACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>
#include "WindChime.h"

// 音频反馈初始化
void AudioFeedbackInit(void);

// 播放事件音效
void PlayEventSound(data_source_t source, int32_t intensity);

// 更新环境音效
void UpdateAmbientSound(int16_t wind_speed);

// 设置音量
void SetAudioVolume(uint8_t volume);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*AUDIO_FEEDBACK_H*/