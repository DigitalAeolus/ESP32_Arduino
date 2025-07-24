#include <Arduino.h>
#include <lvgl.h>
#include "DataSimulator.h"
#include "WindChime.h"
#include "WindChimeConfig.h"

static lv_timer_t * github_timer = NULL;
static lv_timer_t * wiki_timer = NULL;
static lv_timer_t * weather_timer = NULL;

// 模拟数据
static const char* github_repos[] = {
    "tensorflow/tensorflow",
    "microsoft/vscode", 
    "facebook/react",
    "torvalds/linux",
    "nodejs/node",
    "python/cpython"
};

static const char* wiki_articles[] = {
    "Artificial Intelligence",
    "Climate Change",
    "Philosophy", 
    "Mathematics",
    "History",
    "Science"
};

static const char* github_users[] = {
    "developer123",
    "coder_pro",
    "tech_ninja",
    "code_master",
    "dev_guru"
};

// 定时器回调函数
static void github_event_callback(lv_timer_t * timer)
{
    wind_chime_event_t event;
    event.source = DATA_SOURCE_GITHUB;
    event.timestamp = lv_tick_get();
    event.intensity = 20 + (rand() % 60);  // 20-80
    event.color_hash = rand();
    
    int repo_idx = rand() % (sizeof(github_repos) / sizeof(github_repos[0]));
    int user_idx = rand() % (sizeof(github_users) / sizeof(github_users[0]));
    
    snprintf(event.description, sizeof(event.description), 
             "%s pushed to %s", github_users[user_idx], github_repos[repo_idx]);
    
    WindChimeAddEvent(&event);
}

static void wiki_event_callback(lv_timer_t * timer)
{
    wind_chime_event_t event;
    event.source = DATA_SOURCE_WIKIPEDIA;
    event.timestamp = lv_tick_get();
    event.intensity = 15 + (rand() % 50);  // 15-65
    event.color_hash = rand();
    
    int article_idx = rand() % (sizeof(wiki_articles) / sizeof(wiki_articles[0]));
    
    snprintf(event.description, sizeof(event.description), 
             "Anonymous edited '%s'", wiki_articles[article_idx]);
    
    WindChimeAddEvent(&event);
}

static void weather_update_callback(lv_timer_t * timer)
{
    // 模拟风速和温度变化
    static int16_t wind_speed = 5;
    static int16_t temperature = 20;
    
    // 随机变化
    wind_speed += (rand() % 6) - 3;  // -3 to +3
    temperature += (rand() % 4) - 2; // -2 to +2
    
    // 限制范围
    if (wind_speed < 0) wind_speed = 0;
    if (wind_speed > 30) wind_speed = 30;
    if (temperature < -10) temperature = -10;
    if (temperature > 40) temperature = 40;
    
    WindChimeUpdateWeather(wind_speed, temperature);
    
    // 偶尔创建天气事件
    if (rand() % 10 == 0) {  // 10% 概率
        wind_chime_event_t event;
        event.source = DATA_SOURCE_WEATHER;
        event.timestamp = lv_tick_get();
        event.intensity = wind_speed * 2;
        event.color_hash = rand();
        
        snprintf(event.description, sizeof(event.description), 
                 "Wind: %d km/h, Temp: %d°C", wind_speed, temperature);
        
        WindChimeAddEvent(&event);
    }
}

void DataSimulatorInit(void)
{
    // 初始化随机数种子
    srand(lv_tick_get());
}

void DataSimulatorStart(void)
{
    // 启动GitHub事件模拟器
    if (!github_timer) {
        uint32_t interval = WINDCHIME_GITHUB_INTERVAL_MIN + 
                           (rand() % (WINDCHIME_GITHUB_INTERVAL_MAX - WINDCHIME_GITHUB_INTERVAL_MIN));
        github_timer = lv_timer_create(github_event_callback, interval, NULL);
    }
    
    // 启动Wikipedia事件模拟器
    if (!wiki_timer) {
        uint32_t interval = WINDCHIME_WIKI_INTERVAL_MIN + 
                           (rand() % (WINDCHIME_WIKI_INTERVAL_MAX - WINDCHIME_WIKI_INTERVAL_MIN));
        wiki_timer = lv_timer_create(wiki_event_callback, interval, NULL);
    }
    
    // 启动天气更新模拟器
    if (!weather_timer) {
        weather_timer = lv_timer_create(weather_update_callback, WINDCHIME_WEATHER_INTERVAL, NULL);
    }
}

void DataSimulatorStop(void)
{
    if (github_timer) {
        lv_timer_del(github_timer);
        github_timer = NULL;
    }
    
    if (wiki_timer) {
        lv_timer_del(wiki_timer);
        wiki_timer = NULL;
    }
    
    if (weather_timer) {
        lv_timer_del(weather_timer);
        weather_timer = NULL;
    }
}

void SimulateGitHubEvent(void)
{
    github_event_callback(NULL);
}

void SimulateWikipediaEvent(void)
{
    wiki_event_callback(NULL);
}

void SimulateWeatherUpdate(void)
{
    weather_update_callback(NULL);
}