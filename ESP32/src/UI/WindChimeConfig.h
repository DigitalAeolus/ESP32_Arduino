#ifndef WINDCHIME_CONFIG_H
#define WINDCHIME_CONFIG_H

// 视觉效果配置
#define WINDCHIME_MAX_PARTICLES 30        // 减少粒子数量以提高性能
#define WINDCHIME_MAX_RIPPLES 8           // 减少涟漪数量
#define WINDCHIME_ANIMATION_FPS 20        // 降低帧率以节省资源

// 音频配置
#define WINDCHIME_BUZZER_PIN 42           // 蜂鸣器引脚
#define WINDCHIME_DEFAULT_VOLUME 50       // 默认音量

// 颜色主题
#define WINDCHIME_BG_COLOR 0x0a0a0a       // 背景色
#define WINDCHIME_GITHUB_COLOR 0x4A90E2   // GitHub事件颜色
#define WINDCHIME_WIKI_COLOR 0x7B68EE     // Wikipedia事件颜色
#define WINDCHIME_WEATHER_COLOR 0x50C878  // 天气事件颜色

// 数据模拟配置
#define WINDCHIME_GITHUB_INTERVAL_MIN 2000    // GitHub事件最小间隔(ms)
#define WINDCHIME_GITHUB_INTERVAL_MAX 8000    // GitHub事件最大间隔(ms)
#define WINDCHIME_WIKI_INTERVAL_MIN 3000      // Wiki事件最小间隔(ms)
#define WINDCHIME_WIKI_INTERVAL_MAX 10000     // Wiki事件最大间隔(ms)
#define WINDCHIME_WEATHER_INTERVAL 30000      // 天气更新间隔(ms)

// 性能优化
#define WINDCHIME_ENABLE_AUDIO true           // 是否启用音频
#define WINDCHIME_ENABLE_PARTICLES true       // 是否启用粒子效果
#define WINDCHIME_ENABLE_RIPPLES true         // 是否启用涟漪效果

#endif /*WINDCHIME_CONFIG_H*/