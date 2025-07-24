#include <Arduino.h>
#include <lvgl.h>
#include "WindChimeSimple.h"

lv_obj_t * windchime_simple_screen = NULL;
static lv_obj_t * info_label = NULL;
static lv_obj_t * center_circle = NULL;
static lv_obj_t * particle_container = NULL;

// 简单的粒子对象数组
#define MAX_SIMPLE_PARTICLES 10
static lv_obj_t * particles[MAX_SIMPLE_PARTICLES];
static uint32_t particle_timers[MAX_SIMPLE_PARTICLES];

static void create_particle_effect(int16_t x, int16_t y, lv_color_t color)
{
    // 找到空闲的粒子
    for (int i = 0; i < MAX_SIMPLE_PARTICLES; i++) {
        if (particle_timers[i] == 0) {
            // 创建粒子
            lv_obj_clear_flag(particles[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(particles[i], x - 5, y - 5);
            lv_obj_set_style_bg_color(particles[i], color, LV_PART_MAIN);
            
            // 设置生命周期
            particle_timers[i] = lv_tick_get() + 2000; // 2秒生命周期
            break;
        }
    }
}

static void update_particles(void)
{
    uint32_t now = lv_tick_get();
    
    for (int i = 0; i < MAX_SIMPLE_PARTICLES; i++) {
        if (particle_timers[i] > 0) {
            if (now >= particle_timers[i]) {
                // 粒子生命结束
                lv_obj_add_flag(particles[i], LV_OBJ_FLAG_HIDDEN);
                particle_timers[i] = 0;
            } else {
                // 更新透明度
                uint32_t remaining = particle_timers[i] - now;
                uint8_t alpha = (remaining * 255) / 2000;
                lv_obj_set_style_bg_opa(particles[i], alpha, LV_PART_MAIN);
            }
        }
    }
}

static void animation_timer_cb(lv_timer_t * timer)
{
    update_particles();
}

void WindChimeSimpleCreate(lv_event_cb_t event_cb)
{
    windchime_simple_screen = lv_obj_create(NULL);
    
    // 设置深色背景
    lv_obj_set_style_bg_color(windchime_simple_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(windchime_simple_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // 创建导航栏
    lv_obj_t * nav = lv_obj_create(windchime_simple_screen);
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
    
    // Title
    lv_obj_t * title = lv_label_create(nav);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(title, "Digital Wind Chime");
    
    // 创建中心圆圈
    center_circle = lv_obj_create(windchime_simple_screen);
    lv_obj_set_size(center_circle, 100, 100);
    lv_obj_center(center_circle);
    lv_obj_set_style_radius(center_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(center_circle, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(center_circle, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(center_circle, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(center_circle, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    
    // 创建信息标签
    info_label = lv_label_create(windchime_simple_screen);
    lv_obj_set_style_text_color(info_label, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_RIGHT, -10, -60);
    lv_label_set_text(info_label, "Listening to the world...");
    
    // 创建粒子对象
    for (int i = 0; i < MAX_SIMPLE_PARTICLES; i++) {
        particles[i] = lv_obj_create(windchime_simple_screen);
        lv_obj_set_size(particles[i], 10, 10);
        lv_obj_set_style_radius(particles[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(particles[i], 0, LV_PART_MAIN);
        lv_obj_add_flag(particles[i], LV_OBJ_FLAG_HIDDEN);
        particle_timers[i] = 0;
    }
    
    // 启动动画定时器
    lv_timer_create(animation_timer_cb, 50, NULL);  // 20 FPS
}

void WindChimeSimpleAddEvent(const char* source, int intensity)
{
    // 更新信息标签
    char info_text[64];
    snprintf(info_text, sizeof(info_text), "[%s] Event (intensity: %d)", source, intensity);
    lv_label_set_text(info_label, info_text);
    
    // 根据数据源选择颜色
    lv_color_t color = lv_color_make(0x4A, 0x90, 0xE2); // 默认蓝色
    if (strcmp(source, "GitHub") == 0) {
        color = lv_color_make(0x4A, 0x90, 0xE2); // 蓝色
    } else if (strcmp(source, "Wikipedia") == 0) {
        color = lv_color_make(0x7B, 0x68, 0xEE); // 紫色
    } else if (strcmp(source, "Weather") == 0) {
        color = lv_color_make(0x50, 0xC8, 0x78); // 绿色
    }
    
    // 更新中心圆圈颜色
    lv_obj_set_style_border_color(center_circle, color, LV_PART_MAIN);
    
    // 创建粒子效果
    int16_t center_x = 240; // 屏幕中心
    int16_t center_y = 240;
    
    for (int i = 0; i < 3; i++) {
        int16_t x = center_x + (rand() % 100) - 50;
        int16_t y = center_y + (rand() % 100) - 50;
        create_particle_effect(x, y, color);
    }
    
    // 3秒后恢复默认文本
    static lv_timer_t * info_timer = NULL;
    if (info_timer) {
        lv_timer_del(info_timer);
    }
    
    // 暂时不使用定时器恢复文本，避免lambda表达式问题
    // info_timer = lv_timer_create(info_restore_callback, 3000, NULL);
}