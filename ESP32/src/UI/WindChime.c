#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include "WindChime.h"
#include "WindChimeConfig.h"
#include "Screenbase.h"
#include "AudioFeedback.h"

// 配置常量
#define MAX_PARTICLES WINDCHIME_MAX_PARTICLES
#define MAX_RIPPLES WINDCHIME_MAX_RIPPLES
#define MAX_EVENTS_HISTORY 20
#define CANVAS_WIDTH 480
#define CANVAS_HEIGHT 480
#define CENTER_X (CANVAS_WIDTH / 2)
#define CENTER_Y (CANVAS_HEIGHT / 2)

// 全局变量
lv_obj_t * windchime_screen = NULL;
static lv_obj_t * main_canvas = NULL;
static lv_obj_t * info_label = NULL;
static lv_obj_t * volume_bar = NULL;
static lv_obj_t * center_orb = NULL;

// 动画和效果数据
static particle_t particles[MAX_PARTICLES];
static ripple_t ripples[MAX_RIPPLES];
static wind_chime_event_t event_history[MAX_EVENTS_HISTORY];
static uint8_t event_count = 0;
static lv_timer_t * animation_timer = NULL;

// 环境参数
static int16_t current_wind_speed = 0;
static int16_t current_temperature = 20;
static uint8_t audio_volume = 50;
static uint32_t last_event_time = 0;

// 颜色主题 - 使用LV_COLOR_MAKE宏来避免初始化问题
static const lv_color_t source_colors[DATA_SOURCE_MAX] = {
    LV_COLOR_MAKE(0x4A, 0x90, 0xE2),   // GitHub - 蓝色
    LV_COLOR_MAKE(0x7B, 0x68, 0xEE),   // Wikipedia - 紫色  
    LV_COLOR_MAKE(0x50, 0xC8, 0x78)    // Weather - 绿色
};

// 前向声明
static void animation_callback(lv_timer_t * timer);
static void create_ripple(int16_t x, int16_t y, lv_color_t color, uint16_t max_radius);
static void create_particles(int16_t x, int16_t y, lv_color_t color, uint8_t count);
static void update_particles(void);
static void update_ripples(void);
static void update_center_orb(void);
static void show_event_info(wind_chime_event_t* event);
static void create_visual_objects(void);
static void update_visual_objects(void);
static void info_timer_callback(lv_timer_t * timer);

void WindChimeScreenCreate(lv_event_cb_t event_cb)
{
    windchime_screen = lv_obj_create(NULL);
    
    // 设置深色背景
    lv_obj_set_style_bg_color(windchime_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(windchime_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // 创建导航栏
    lv_obj_t * nav = lv_obj_create(windchime_screen);
    lv_obj_set_style_pad_all(nav, 6, LV_PART_MAIN);
    lv_obj_set_size(nav, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav, lv_color_make(0x1a, 0x1a, 0x1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(nav, LV_OPA_80, LV_PART_MAIN);
    
    // Back button
    lv_obj_t * btn_back = lv_btn_create(nav);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_size(btn_back, lv_pct(30), LV_SIZE_CONTENT);
    lv_obj_add_event_cb(btn_back, event_cb, LV_EVENT_CLICKED, (void *)0);
    lv_obj_set_style_bg_color(btn_back, lv_palette_darken(LV_PALETTE_RED,2), LV_PART_MAIN);
    
    lv_obj_t * label_back = lv_label_create(btn_back);
    lv_obj_align(label_back, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_back, "Back");
    
    // Control button
    lv_obj_t * btn_control = lv_btn_create(nav);
    lv_obj_align(btn_control, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_size(btn_control, lv_pct(30), LV_SIZE_CONTENT);
    lv_obj_add_event_cb(btn_control, event_cb, LV_EVENT_CLICKED, (void *)1);
    lv_obj_set_style_bg_color(btn_control, lv_palette_darken(LV_PALETTE_GREEN,2), LV_PART_MAIN);
    
    lv_obj_t * label_control = lv_label_create(btn_control);
    lv_obj_align(label_control, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_control, "Control");
    
    // Title
    lv_obj_t * title = lv_label_create(nav);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(title, "Digital Wind Chime");
    
    lv_obj_update_layout(nav);
    int nav_height = lv_obj_get_height(nav);
    
    // 创建主画布 (调整大小以适应导航栏)
    main_canvas = lv_canvas_create(windchime_screen);
    lv_obj_set_size(main_canvas, CANVAS_WIDTH, CANVAS_HEIGHT - nav_height);
    lv_obj_align(main_canvas, LV_ALIGN_TOP_MID, 0, 0);
    
    // 设置画布背景
    lv_obj_set_style_bg_color(main_canvas, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_canvas, LV_OPA_COVER, LV_PART_MAIN);
    
    // 创建中心球体
    center_orb = lv_obj_create(windchime_screen);
    lv_obj_set_size(center_orb, 80, 80);
    lv_obj_align(center_orb, LV_ALIGN_CENTER, 0, -nav_height/2);
    lv_obj_set_style_radius(center_orb, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(center_orb, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(center_orb, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(center_orb, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(center_orb, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(center_orb, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(center_orb, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(center_orb, LV_OPA_50, LV_PART_MAIN);
    
    // 创建信息显示标签
    info_label = lv_label_create(windchime_screen);
    lv_obj_set_style_text_color(info_label, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_label_set_text(info_label, "Listening to the world...");
    
    // 创建音量指示器
    volume_bar = lv_bar_create(windchime_screen);
    lv_obj_set_size(volume_bar, 4, 60);
    lv_obj_align(volume_bar, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(volume_bar, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_bar, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_INDICATOR);
    lv_bar_set_range(volume_bar, 0, 100);
    lv_bar_set_value(volume_bar, audio_volume, LV_ANIM_OFF);
    
    // 初始化粒子和涟漪
    memset(particles, 0, sizeof(particles));
    memset(ripples, 0, sizeof(ripples));
    memset(event_history, 0, sizeof(event_history));
    
    // 创建视觉对象
    create_visual_objects();
    
    // 启动动画定时器
    WindChimeStartAnimation();
}void
 WindChimeAddEvent(wind_chime_event_t* event)
{
    if (!event) return;
    
    // 保存事件到历史记录
    event_history[event_count % MAX_EVENTS_HISTORY] = *event;
    event_count++;
    
    // 根据数据源确定位置和效果
    int16_t x, y;
    lv_color_t color = source_colors[event->source];
    
    // 根据事件强度和源类型计算位置
    switch(event->source) {
        case DATA_SOURCE_GITHUB:
            x = CENTER_X + (rand() % 200) - 100;
            y = CENTER_Y + (rand() % 200) - 100;
            break;
        case DATA_SOURCE_WIKIPEDIA:
            x = CENTER_X + (rand() % 160) - 80;
            y = CENTER_Y + (rand() % 160) - 80;
            break;
        case DATA_SOURCE_WEATHER:
            x = CENTER_X;
            y = CENTER_Y;
            break;
        default:
            x = CENTER_X;
            y = CENTER_Y;
            break;
    }
    
    // 创建涟漪效果
    uint16_t ripple_size = 50 + (event->intensity * 2);
    create_ripple(x, y, color, ripple_size);
    
    // 创建粒子效果
    uint8_t particle_count = 3 + (event->intensity / 20);
    create_particles(x, y, color, particle_count);
    
    // 播放音效
    PlayEventSound(event->source, event->intensity);
    
    // 显示事件信息
    show_event_info(event);
    
    // 更新中心球体颜色
    lv_obj_set_style_shadow_color(center_orb, color, LV_PART_MAIN);
    
    last_event_time = lv_tick_get();
}

void WindChimeUpdateWeather(int16_t wind_speed, int16_t temperature)
{
    current_wind_speed = wind_speed;
    current_temperature = temperature;
    
    // 更新环境音效
    UpdateAmbientSound(wind_speed);
    
    // 风速影响粒子系统的全局行为
    // 这里可以调整粒子的生命周期和运动速度
}

void WindChimeSetVolume(uint8_t volume)
{
    audio_volume = volume;
    lv_bar_set_value(volume_bar, volume, LV_ANIM_ON);
    SetAudioVolume(volume);
}

static void create_ripple(int16_t x, int16_t y, lv_color_t color, uint16_t max_radius)
{
    // 找到空闲的涟漪槽位
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) {
            ripples[i].x = x;
            ripples[i].y = y;
            ripples[i].radius = 0;
            ripples[i].max_radius = max_radius;
            ripples[i].alpha = 255;
            ripples[i].color = color;
            ripples[i].active = true;
            break;
        }
    }
}

static void create_particles(int16_t x, int16_t y, lv_color_t color, uint8_t count)
{
    int created = 0;
    for (int i = 0; i < MAX_PARTICLES && created < count; i++) {
        if (particles[i].life == 0) {
            particles[i].x = x;
            particles[i].y = y;
            
            // 随机速度
            float angle = (rand() % 360) * M_PI / 180.0f;
            float speed = 1 + (rand() % 3);
            particles[i].vx = (int16_t)(cos(angle) * speed);
            particles[i].vy = (int16_t)(sin(angle) * speed);
            
            particles[i].life = 60 + (rand() % 60);  // 1-2秒生命周期
            particles[i].max_life = particles[i].life;
            particles[i].color = color;
            particles[i].size = 2 + (rand() % 3);
            created++;
        }
    }
}

static void update_particles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            // 更新位置
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            
            // 风速影响
            particles[i].x += (current_wind_speed / 10);
            
            // 重力效果
            particles[i].vy += 1;
            
            // 边界检查
            if (particles[i].x < 0 || particles[i].x >= CANVAS_WIDTH ||
                particles[i].y < 0 || particles[i].y >= CANVAS_HEIGHT) {
                particles[i].life = 0;
                continue;
            }
            
            // 减少生命值
            particles[i].life--;
        }
    }
}

static void update_ripples(void)
{
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            ripples[i].radius += 3;
            ripples[i].alpha = (uint8_t)(255 * (1.0f - (float)ripples[i].radius / ripples[i].max_radius));
            
            if (ripples[i].radius >= ripples[i].max_radius) {
                ripples[i].active = false;
            }
        }
    }
}

static void update_center_orb(void)
{
    // 根据最近事件的时间调整中心球体的亮度
    uint32_t time_since_event = lv_tick_get() - last_event_time;
    
    if (time_since_event < 2000) {  // 2秒内有事件
        uint8_t brightness = 255 - (time_since_event * 255 / 2000);
        lv_obj_set_style_shadow_opa(center_orb, brightness, LV_PART_MAIN);
    } else {
        lv_obj_set_style_shadow_opa(center_orb, LV_OPA_20, LV_PART_MAIN);
    }
    
    // 风速影响球体的抖动（通过微调位置）
    if (current_wind_speed > 5) {
        int16_t shake_range = current_wind_speed / 5;
        if (shake_range < 1) shake_range = 1;
        int16_t shake_x = (rand() % (shake_range * 2)) - shake_range;
        int16_t shake_y = (rand() % (shake_range * 2)) - shake_range;
        lv_obj_align(center_orb, LV_ALIGN_CENTER, shake_x, shake_y - 30);  // -30 to account for nav bar
    } else {
        lv_obj_align(center_orb, LV_ALIGN_CENTER, 0, -30);
    }
}

static void show_event_info(wind_chime_event_t* event)
{
    char info_text[128];
    const char* source_names[] = {"GitHub", "Wikipedia", "Weather"};
    
    snprintf(info_text, sizeof(info_text), "[%s] %s", 
             source_names[event->source], event->description);
    
    lv_label_set_text(info_label, info_text);
    
    // 设置标签颜色匹配数据源
    lv_obj_set_style_text_color(info_label, source_colors[event->source], LV_PART_MAIN);
    
    // 3秒后恢复默认文本
    static lv_timer_t * info_timer = NULL;
    if (info_timer) {
        lv_timer_del(info_timer);
    }
    info_timer = lv_timer_create(info_timer_callback, 3000, NULL);
}

// 使用LVGL对象来创建视觉效果，而不是直接绘制到画布
static lv_obj_t * particle_objects[MAX_PARTICLES];
static lv_obj_t * ripple_objects[MAX_RIPPLES];

static void create_visual_objects(void)
{
    // 创建粒子对象
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_objects[i] = lv_obj_create(main_canvas);
        lv_obj_set_size(particle_objects[i], 4, 4);
        lv_obj_set_style_radius(particle_objects[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(particle_objects[i], 0, LV_PART_MAIN);
        lv_obj_add_flag(particle_objects[i], LV_OBJ_FLAG_HIDDEN);
    }
    
    // 创建涟漪对象
    for (int i = 0; i < MAX_RIPPLES; i++) {
        ripple_objects[i] = lv_obj_create(main_canvas);
        lv_obj_set_size(ripple_objects[i], 20, 20);
        lv_obj_set_style_radius(ripple_objects[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(ripple_objects[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(ripple_objects[i], 2, LV_PART_MAIN);
        lv_obj_add_flag(ripple_objects[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_visual_objects(void)
{
    // 更新粒子对象
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            lv_obj_clear_flag(particle_objects[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(particle_objects[i], particles[i].x - 2, particles[i].y - 2);
            
            uint8_t alpha = (particles[i].life * 255) / particles[i].max_life;
            lv_obj_set_style_bg_color(particle_objects[i], particles[i].color, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(particle_objects[i], alpha, LV_PART_MAIN);
        } else {
            lv_obj_add_flag(particle_objects[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // 更新涟漪对象
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            lv_obj_clear_flag(ripple_objects[i], LV_OBJ_FLAG_HIDDEN);
            
            int16_t size = ripples[i].radius * 2;
            lv_obj_set_size(ripple_objects[i], size, size);
            lv_obj_set_pos(ripple_objects[i], 
                          ripples[i].x - ripples[i].radius, 
                          ripples[i].y - ripples[i].radius);
            
            lv_obj_set_style_border_color(ripple_objects[i], ripples[i].color, LV_PART_MAIN);
            lv_obj_set_style_border_opa(ripple_objects[i], ripples[i].alpha, LV_PART_MAIN);
        } else {
            lv_obj_add_flag(ripple_objects[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void animation_callback(lv_timer_t * timer)
{
    update_particles();
    update_ripples();
    update_center_orb();
    update_visual_objects();
}

void WindChimeStartAnimation(void)
{
    if (!animation_timer) {
        uint32_t period = 1000 / WINDCHIME_ANIMATION_FPS;
        animation_timer = lv_timer_create(animation_callback, period, NULL);
    }
}

// 信息标签定时器回调
static void info_timer_callback(lv_timer_t * timer)
{
    lv_label_set_text(info_label, "Listening to the world...");
    lv_obj_set_style_text_color(info_label, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_timer_del(timer);
}

void WindChimeStopAnimation(void)
{
    if (animation_timer) {
        lv_timer_del(animation_timer);
        animation_timer = NULL;
    }
}