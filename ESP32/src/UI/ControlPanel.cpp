#include <Arduino.h>
#include <lvgl.h>
#include "ControlPanel.h"
#include "WindChime.h"
#include "DataSimulator.h"
#include "WiFiConfig.h"
#include "AudioFeedback.h"
#include "../Core/WiFiManager.h"
#include "../Core/MQTTManager.h"

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
                if (MQTTManager_IsConnected()) {
                    lv_label_set_text(status_label, "MQTT connected - using real data");
                } else if (DataSimulatorIsRunning()) {
                    DataSimulatorStop();
                    simulator_running = false;
                    lv_obj_t * label = lv_obj_get_child(btn, 0);
                    lv_label_set_text(label, "Start Auto Sim");
                    lv_label_set_text(status_label, "Auto simulation stopped");
                } else {
                    DataSimulatorStart();
                    simulator_running = DataSimulatorIsRunning();
                    if (simulator_running) {
                        lv_obj_t * label = lv_obj_get_child(btn, 0);
                        lv_label_set_text(label, "Stop Auto Sim");
                        lv_label_set_text(status_label, "Auto simulation started");
                    } else {
                        lv_label_set_text(status_label, "Cannot start - MQTT mode active");
                    }
                }
                break;
                
            case 5: // WiFi Config
                WiFiConfigShow();
                lv_label_set_text(status_label, "Opening WiFi configuration...");
                break;
        }
    }
}

// 音量滑块事件处理
static void volume_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        SetAudioVolume((uint8_t)value);
        
        // 更新音量值标签
        lv_obj_t * volume_cont = lv_obj_get_parent(slider);
        lv_obj_t * volume_value_label = lv_obj_get_child(volume_cont, 2); // 第三个子对象
        if (volume_value_label) {
            char value_text[8];
            snprintf(value_text, sizeof(value_text), "%d%%", (int)value);
            lv_label_set_text(volume_value_label, value_text);
        }
        
        char status_text[64];
        snprintf(status_text, sizeof(status_text), "Volume set to %d%%", (int)value);
        lv_label_set_text(status_label, status_text);
    }
}

static lv_obj_t * create_button(lv_obj_t * parent, const char * text, int btn_id)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_add_event_cb(btn, btn_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)btn_id);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    // 根据按钮类型设置不同颜色
    lv_color_t btn_color;
    switch (btn_id) {
        case 0: // Back - 灰色
            btn_color = lv_color_make(0x5a, 0x5a, 0x5a);
            break;
        case 1: // GitHub - 绿色
            btn_color = lv_color_make(0x28, 0xa7, 0x45);
            break;
        case 2: // Wikipedia - 蓝色
            btn_color = lv_color_make(0x00, 0x6b, 0xb3);
            break;
        case 3: // Weather - 橙色
            btn_color = lv_color_make(0xff, 0x8c, 0x00);
            break;
        case 4: // Auto Sim - 紫色
            btn_color = lv_color_make(0x8e, 0x44, 0xad);
            break;
        case 5: // WiFi - 青色
            btn_color = lv_color_make(0x17, 0xa2, 0xb8);
            break;
        default:
            btn_color = lv_color_make(0x4A, 0x90, 0xE2);
            break;
    }
    
    // 按钮样式
    lv_obj_set_style_bg_color(btn, btn_color, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_darken(btn_color, LV_OPA_20), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_lighten(btn_color, LV_OPA_30), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_white(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, LV_PART_MAIN);
    
    // 添加渐变效果
    lv_obj_set_style_bg_grad_color(btn, lv_color_lighten(btn_color, LV_OPA_20), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, LV_PART_MAIN);
    
    return btn;
}

extern "C" {

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
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
    
    // 创建WiFi状态指示器
    lv_obj_t * wifi_status_cont = lv_obj_create(control_panel_screen);
    lv_obj_set_size(wifi_status_cont, lv_pct(95), 35);
    lv_obj_align_to(wifi_status_cont, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(wifi_status_cont, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_status_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(wifi_status_cont, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_status_cont, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_status_cont, 8, LV_PART_MAIN);
    
    lv_obj_t * wifi_status_label = lv_label_create(wifi_status_cont);
    // 获取当前WiFi和MQTT状态
    wifi_status_t wifi_status = WiFiManager_GetStatus();
    mqtt_status_t mqtt_status = MQTTManager_GetStatus();
    
    char status_text[128];
    lv_color_t status_color;
    
    if (wifi_status == WIFI_STATUS_CONNECTED) {
        const char* ssid = WiFiManager_GetConnectedSSID();
        if (mqtt_status == MQTT_STATUS_CONNECTED) {
            snprintf(status_text, sizeof(status_text), "WiFi: %s | MQTT: Connected", ssid);
            status_color = lv_color_make(0x66, 0xff, 0x66); // 绿色
        } else if (mqtt_status == MQTT_STATUS_CONNECTING || mqtt_status == MQTT_STATUS_RECONNECTING) {
            snprintf(status_text, sizeof(status_text), "WiFi: %s | MQTT: Connecting...", ssid);
            status_color = lv_color_make(0xff, 0xff, 0x66); // 黄色
        } else {
            snprintf(status_text, sizeof(status_text), "WiFi: %s | MQTT: Disconnected", ssid);
            status_color = lv_color_make(0xff, 0xaa, 0x66); // 橙色
        }
    } else {
        snprintf(status_text, sizeof(status_text), "WiFi: Not connected | MQTT: N/A");
        status_color = lv_color_make(0xff, 0x66, 0x66); // 红色
    }
    
    lv_label_set_text(wifi_status_label, status_text);
    lv_obj_set_style_text_color(wifi_status_label, status_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(wifi_status_label);
    
    // 创建主容器
    lv_obj_t * cont = lv_obj_create(control_panel_screen);
    lv_obj_set_size(cont, lv_pct(95), 380);
    lv_obj_align_to(cont, wifi_status_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(cont, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(cont, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(cont, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(cont, LV_OPA_30, LV_PART_MAIN);
    
    // 创建网格布局容器
    lv_obj_t * grid_cont = lv_obj_create(cont);
    lv_obj_set_size(grid_cont, lv_pct(100), 200);
    lv_obj_align(grid_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(grid_cont, 0, LV_PART_MAIN);
    
    // 设置网格布局 - 3列2行
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid_cont, col_dsc, row_dsc);
    
    // 创建按钮 - 使用网格布局
    lv_obj_t * btn_github = create_button(grid_cont, "GitHub Event", 1);
    lv_obj_set_grid_cell(btn_github, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    
    lv_obj_t * btn_wiki = create_button(grid_cont, "Wikipedia", 2);
    lv_obj_set_grid_cell(btn_wiki, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    
    lv_obj_t * btn_weather = create_button(grid_cont, "Weather", 3);
    lv_obj_set_grid_cell(btn_weather, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    
    lv_obj_t * btn_auto = create_button(grid_cont, "Auto Sim", 4);
    lv_obj_set_grid_cell(btn_auto, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    
    lv_obj_t * btn_wifi = create_button(grid_cont, "WiFi Config", 5);
    lv_obj_set_grid_cell(btn_wifi, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    
    lv_obj_t * btn_back = create_button(grid_cont, "Back", 0);
    lv_obj_set_grid_cell(btn_back, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    
    // 音量控制区域
    lv_obj_t * volume_cont = lv_obj_create(cont);
    lv_obj_set_size(volume_cont, lv_pct(100), 60);
    lv_obj_align_to(volume_cont, grid_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_color(volume_cont, lv_color_make(0x3a, 0x3a, 0x3a), LV_PART_MAIN);
    lv_obj_set_style_border_width(volume_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(volume_cont, lv_color_make(0x5a, 0x5a, 0x5a), LV_PART_MAIN);
    lv_obj_set_style_radius(volume_cont, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(volume_cont, 15, LV_PART_MAIN);
    
    lv_obj_t * volume_label = lv_label_create(volume_cont);
    lv_label_set_text(volume_label, "Volume Control:");
    lv_obj_set_style_text_color(volume_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    volume_slider = lv_slider_create(volume_cont);
    lv_obj_set_size(volume_slider, lv_pct(70), 20);
    lv_obj_align_to(volume_slider, volume_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_slider_set_range(volume_slider, 0, 100);
    
    // 加载保存的音量设置
    uint8_t saved_volume = GetAudioVolume();
    lv_slider_set_value(volume_slider, saved_volume, LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_slider, volume_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // 音量值标签
    lv_obj_t * volume_value_label = lv_label_create(volume_cont);
    char volume_text[8];
    snprintf(volume_text, sizeof(volume_text), "%d%%", saved_volume);
    lv_label_set_text(volume_value_label, volume_text);
    lv_obj_set_style_text_color(volume_value_label, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN);
    lv_obj_set_style_text_font(volume_value_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align_to(volume_value_label, volume_slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    // 滑块样式
    lv_obj_set_style_bg_color(volume_slider, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_slider, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volume_slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_radius(volume_slider, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(volume_slider, 10, LV_PART_INDICATOR);
    lv_obj_set_style_radius(volume_slider, 50, LV_PART_KNOB);
    
    // 状态标签
    status_label = lv_label_create(cont);
    lv_label_set_text(status_label, "Ready to simulate events...");
    lv_obj_set_style_text_color(status_label, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    
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

} // extern "C"