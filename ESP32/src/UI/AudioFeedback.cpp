#include <Arduino.h>
#include <Preferences.h>
#include "AudioFeedback.h"
#include "WindChimeConfig.h"

// 音频配置
#define BUZZER_PIN WINDCHIME_BUZZER_PIN
#define PWM_CHANNEL 0
#define PWM_FREQUENCY 2000
#define PWM_RESOLUTION 8

static uint8_t current_volume = WINDCHIME_DEFAULT_VOLUME;
static bool audio_enabled = true;
static Preferences audio_prefs;

// 不同数据源的音调频率
static const uint16_t source_frequencies[DATA_SOURCE_MAX] = {
    880,   // GitHub - A5 (清脆的金属声)
    659,   // Wikipedia - E5 (温暖的木质声)
    523    // Weather - C5 (自然的风声)
};

extern "C" {

void AudioFeedbackInit(void)
{
    // 初始化蜂鸣器引脚
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // 初始化音频设置存储
    audio_prefs.begin("audio_config", false);
    
    // 加载保存的音量设置
    current_volume = LoadAudioVolume();
}

void PlayEventSound(data_source_t source, int32_t intensity)
{
    if (!audio_enabled || current_volume == 0) return;
    
    if (source >= DATA_SOURCE_MAX) return;
    
    // 简单的蜂鸣器控制 - 使用基本的digitalWrite
    uint16_t duration = 100 + (intensity * 2);  // 100-300ms
    
    // 简单的蜂鸣声
    for (int i = 0; i < 10; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(5);
        digitalWrite(BUZZER_PIN, LOW);
        delay(5);
    }
}

void UpdateAmbientSound(int16_t wind_speed)
{
    // 根据风速调整背景音效
    if (wind_speed > 10 && audio_enabled && current_volume > 0) {
        // 简单的风声模拟
        for (int i = 0; i < wind_speed / 5; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(2);
            digitalWrite(BUZZER_PIN, LOW);
            delay(3);
        }
    }
}

void SetAudioVolume(uint8_t volume)
{
    current_volume = volume;
    SaveAudioVolume(volume);
}

uint8_t GetAudioVolume(void)
{
    return current_volume;
}

void SaveAudioVolume(uint8_t volume)
{
    audio_prefs.putUChar("volume", volume);
    Serial.printf("AudioFeedback: Volume saved: %d\n", volume);
}

uint8_t LoadAudioVolume(void)
{
    uint8_t volume = audio_prefs.getUChar("volume", WINDCHIME_DEFAULT_VOLUME);
    Serial.printf("AudioFeedback: Volume loaded: %d\n", volume);
    return volume;
}

} // extern "C"