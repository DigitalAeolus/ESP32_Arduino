#include <Arduino.h>
#include <lvgl.h>
#include "ControlPanel.h"
#include "WindChime.h"
#include "DataSimulator.h"

lv_obj_t * control_panel_screen = NULL;
static lv_obj_t * volume_slider = NULL;
static lv_obj_t * status_label = NULL;
static bool simulator_running = false;

// 按钮事件处理
static void btn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_current_target_obj(e);
    
    if (code == LV_EVENT_CLICKED) {
        void * user_data = lv_event_get_user_data(e);
        int btn_id = (int)(uintptr_t)user_data;
        
        switch (btn_id) {
            case 0: // Back to Wind Chime
                lv_screen_load(windchime_screen);
                break;
                
            case 1: // Simulate GitHub Event
                SimulateGitHubEvent();
                lv_label_set_text(status_label, "GitHub event triggered!");
                break;
                
            case 2: // Simulate Wikipedia Event
                SimulateWikipediaEvent();
                lv_label_set_text(status_label, "Wikipedia event triggered!");
                break;
                
            case 3: // Simulate Weather Update
                SimulateWeatherUpdate();
                lv_label_set_text(status_label, "Weather updated!");
                break;
                
            case 4: // Toggle Auto Simulation
                if (simulator_running) {
                    DataSimulatorStop();
                    simulator_running = false;
                    lv_obj_t * label = lv_obj_get_child(btn, 0);
                    lv_label_set_text(label, "Start Auto Sim");
                    lv_label_set_text(status_label, "Auto simulation stopped");
                } else {
                    DataSimulatorStart();
                    simulator_running = true;
                    lv_obj_t * label = lv_obj_get_child(btn, 0);
                    lv_label_set_text(label, "Stop Auto Sim");
                    lv_label_set_text(status_label, "Auto simulation started");
                }
                break;
        }
    }
}

// 音量滑块事件处理
static void volume_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * slider = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        WindChimeSetVolume((uint8_t)value);
        
        char status_text[64];
        snprintf(status_text, sizeof(status_text), "Volume set to %d%%", (int)value);
        lv_label_set_text(status_label, status_text);
    }
}

static lv_obj_t * create_button(lv_obj_t * parent, const char * text, int btn_id)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 180, 50);
    lv_obj_add_event_cb(btn, btn_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)btn_id);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    // 按钮样式
    lv_obj_set_style_bg_color(btn, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);
    
    return btn;
}

void ControlPanelCreate(lv_event_cb_t event_cb)
{
    control_panel_screen = lv_obj_create(NULL);
    
    // 设置深色背景
    lv_obj_set_style_bg_color(control_panel_screen, lv_color_make(0x1a, 0x1a, 0x1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(control_panel_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // 创建标题
    lv_obj_t * title = lv_label_create(control_panel_screen);
    lv_label_set_text(title, "Digital Wind Chime Control");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // 创建主容器
    lv_obj_t * cont = lv_obj_create(control_panel_screen);
    lv_obj_set_size(cont, 400, 350);
    lv_obj_center(cont);
    lv_obj_set_style_bg_color(cont, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 10, LV_PART_MAIN);
    
    // 创建按钮
    lv_obj_t * btn_back = create_button(cont, "Back to Wind Chime", 0);
    lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t * btn_github = create_button(cont, "Trigger GitHub", 1);
    lv_obj_align_to(btn_github, btn_back, LV_ALIGN_OUT_BOTTOM_LEFT, -90, 15);
    
    lv_obj_t * btn_wiki = create_button(cont, "Trigger Wikipedia", 2);
    lv_obj_align_to(btn_wiki, btn_back, LV_ALIGN_OUT_BOTTOM_RIGHT, 90, 15);
    
    lv_obj_t * btn_weather = create_button(cont, "Update Weather", 3);
    lv_obj_align_to(btn_weather, btn_github, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    
    lv_obj_t * btn_auto = create_button(cont, "Start Auto Sim", 4);
    lv_obj_align_to(btn_auto, btn_wiki, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 15);
    
    // 音量控制
    lv_obj_t * volume_label = lv_label_create(cont);
    lv_label_set_text(volume_label, "Volume:");
    lv_obj_set_style_text_color(volume_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align_to(volume_label, btn_weather, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25);
    
    volume_slider = lv_slider_create(cont);
    lv_obj_set_size(volume_slider, 200, 20);
    lv_obj_align_to(volume_slider, volume_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_slider, volume_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // 滑块样式
    lv_obj_set_style_bg_color(volume_slider, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_slider, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volume_slider, lv_color_white(), LV_PART_KNOB);
    
    // 状态标签
    status_label = lv_label_create(cont);
    lv_label_set_text(status_label, "Ready to simulate events...");
    lv_obj_set_style_text_color(status_label, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // 初始化数据模拟器
    DataSimulatorInit();
}

void ControlPanelShow(void)
{
    if (control_panel_screen) {
        lv_screen_load(control_panel_screen);
    }
}

void ControlPanelHide(void)
{
    if (windchime_screen) {
        lv_screen_load(windchime_screen);
    }
}