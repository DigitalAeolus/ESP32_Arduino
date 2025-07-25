#include "WiFiConfig.h"
#include "../Core/WiFiManager.h"
#include "ControlPanel.h"
#include <stdio.h>
#include <string.h>

lv_obj_t * wifi_config_screen = NULL;
static lv_obj_t * network_list = NULL;
static lv_obj_t * status_label = NULL;
static lv_obj_t * current_network_label = NULL;
static lv_obj_t * ip_label = NULL;
static lv_obj_t * password_textarea = NULL;
static lv_obj_t * password_panel = NULL;
static lv_obj_t * keyboard = NULL;
static lv_obj_t * scan_btn = NULL;
static lv_obj_t * connect_btn = NULL;
static lv_obj_t * disconnect_btn = NULL;

static char selected_ssid[33] = {0};
static bool password_panel_visible = false;

// 前向声明
static void network_list_event_handler(lv_event_t * e);
static void password_event_handler(lv_event_t * e);
static void keyboard_event_handler(lv_event_t * e);
static void btn_event_handler(lv_event_t * e);
static void wifi_status_callback(wifi_status_t status, const char* message);
static void wifi_scan_callback(wifi_network_t* networks, int count);
static void show_password_panel(const char* ssid);
static void hide_password_panel(void);
static void create_network_item(lv_obj_t* parent, const char* ssid, int32_t rssi, 
                               wifi_auth_mode_t auth_mode, int index);

// WiFi状态回调
static void wifi_status_callback(wifi_status_t status, const char* message)
{
    if (!wifi_config_screen || !status_label) return;
    
    const char* status_text = message ? message : WiFiManager_GetStatusString();
    bool is_error = (status == WIFI_STATUS_FAILED);
    
    WiFiConfigUpdateStatus(status_text, is_error);
    
    if (status == WIFI_STATUS_CONNECTED) {
        hide_password_panel();
        WiFiConfigRefreshConnection();
        
        // 更新按钮状态
        if (disconnect_btn) {
            lv_obj_clear_flag(disconnect_btn, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (disconnect_btn) {
            lv_obj_add_flag(disconnect_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// WiFi扫描回调
static void wifi_scan_callback(wifi_network_t* networks, int count)
{
    if (!network_list) return;
    
    // 清空现有列表
    lv_obj_clean(network_list);
    
    if (count == 0) {
        lv_obj_t * no_networks = lv_label_create(network_list);
        lv_label_set_text(no_networks, "No networks found");
        lv_obj_set_style_text_color(no_networks, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
        lv_obj_center(no_networks);
        return;
    }
    
    // 添加网络项
    for (int i = 0; i < count && i < 20; i++) { // 最多显示20个网络
        if (!networks[i].is_hidden) { // 跳过隐藏网络
            create_network_item(network_list, networks[i].ssid, networks[i].rssi, 
                              networks[i].auth_mode, i);
        }
    }
    
    if (WiFiManager_GetStatus() == WIFI_STATUS_CONNECTED) {
        WiFiConfigUpdateStatus("Scan completed - Select network to switch", false);
    } else {
        WiFiConfigUpdateStatus("Scan completed", false);
    }
}

// 创建网络列表项
static void create_network_item(lv_obj_t* parent, const char* ssid, int32_t rssi, 
                               wifi_auth_mode_t auth_mode, int index)
{
    lv_obj_t * item = lv_obj_create(parent);
    lv_obj_set_size(item, lv_pct(100), 55);
    lv_obj_set_style_bg_color(item, lv_color_make(0x3a, 0x3a, 0x3a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(item, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(item, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(item, lv_color_make(0x5a, 0x5a, 0x5a), LV_PART_MAIN);
    lv_obj_set_style_radius(item, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(item, 10, LV_PART_MAIN);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, network_list_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)index);
    
    // 创建左侧内容容器
    lv_obj_t * left_cont = lv_obj_create(item);
    lv_obj_set_size(left_cont, lv_pct(75), lv_pct(100));
    lv_obj_align(left_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(left_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(left_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(left_cont, 0, LV_PART_MAIN);
    
    // SSID标签
    lv_obj_t * ssid_label = lv_label_create(left_cont);
    lv_label_set_text(ssid_label, ssid);
    lv_obj_set_style_text_color(ssid_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_long_mode(ssid_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(ssid_label, lv_pct(100));
    
    // 信号强度和加密类型
    char info_text[48];
    const char* auth_str = WiFiManager_GetAuthModeString(auth_mode);
    snprintf(info_text, sizeof(info_text), "%s | %ddBm", auth_str, (int)rssi);
    
    lv_obj_t * info_label = lv_label_create(left_cont);
    lv_label_set_text(info_label, info_text);
    lv_obj_set_style_text_color(info_label, lv_color_make(0xaa, 0xaa, 0xaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(info_label, lv_pct(100));
    
    // 右侧信号强度指示器
    lv_obj_t * signal_cont = lv_obj_create(item);
    lv_obj_set_size(signal_cont, lv_pct(20), lv_pct(100));
    lv_obj_align(signal_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(signal_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(signal_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(signal_cont, 0, LV_PART_MAIN);
    
    // 信号强度图标
    int signal_strength = WiFiManager_GetSignalStrength(rssi);
    const char* signal_icon;
    lv_color_t signal_color;
    
    if (signal_strength >= 3) {
        signal_icon = "●●●";
        signal_color = lv_color_make(0x66, 0xff, 0x66);
    } else if (signal_strength >= 2) {
        signal_icon = "●●○";
        signal_color = lv_color_make(0xff, 0xff, 0x66);
    } else if (signal_strength >= 1) {
        signal_icon = "●○○";
        signal_color = lv_color_make(0xff, 0xaa, 0x66);
    } else {
        signal_icon = "○○○";
        signal_color = lv_color_make(0xff, 0x66, 0x66);
    }
    
    lv_obj_t * signal_label = lv_label_create(signal_cont);
    lv_label_set_text(signal_label, signal_icon);
    lv_obj_set_style_text_color(signal_label, signal_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(signal_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(signal_label);
    
    // 存储SSID到用户数据（使用静态缓冲区避免内存泄漏）
    static char ssid_buffer[20][33]; // 最多20个网络，每个SSID最长32字符
    static int buffer_index = 0;
    
    strncpy(ssid_buffer[buffer_index % 20], ssid, 32);
    ssid_buffer[buffer_index % 20][32] = '\0';
    lv_obj_set_user_data(item, ssid_buffer[buffer_index % 20]);
    buffer_index++;
}

// 网络列表事件处理
static void network_list_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * item = lv_event_get_current_target_obj(e);
    
    if (code == LV_EVENT_CLICKED) {
        char* ssid = (char*)lv_obj_get_user_data(item);
        if (ssid) {
            strncpy(selected_ssid, ssid, sizeof(selected_ssid) - 1);
            selected_ssid[sizeof(selected_ssid) - 1] = '\0';
            
            // 检查是否有保存的配置
            wifi_manager_config_t config;
            bool has_saved_config = WiFiManager_LoadConfig(&config) && 
                                   strcmp(config.ssid, ssid) == 0;
            
            // 检查是否是当前连接的网络
            wifi_status_t current_status = WiFiManager_GetStatus();
            const char* current_ssid = WiFiManager_GetConnectedSSID();
            
            if (current_status == WIFI_STATUS_CONNECTED && strcmp(current_ssid, ssid) == 0) {
                // 已经连接到这个网络
                WiFiConfigUpdateStatus("Already connected to this network", false);
                return;
            }
            
            if (has_saved_config) {
                // 使用保存的配置直接连接（会自动断开当前连接）
                WiFiManager_Connect(config.ssid, config.password, false);
                WiFiConfigUpdateStatus("Switching to saved network...", false);
            } else {
                // 显示密码输入面板
                show_password_panel(ssid);
            }
        }
    }
}

// 显示密码输入面板
static void show_password_panel(const char* ssid)
{
    if (!password_panel) return;
    
    password_panel_visible = true;
    lv_obj_clear_flag(password_panel, LV_OBJ_FLAG_HIDDEN);
    
    // 更新标题
    lv_obj_t * title = lv_obj_get_child(password_panel, 0);
    if (title) {
        char title_text[48];
        snprintf(title_text, sizeof(title_text), "Connect to: %.20s", ssid);
        lv_label_set_text(title, title_text);
    }
    
    // 清空密码输入框并设置焦点
    if (password_textarea) {
        lv_textarea_set_text(password_textarea, "");
        lv_obj_add_state(password_textarea, LV_STATE_FOCUSED);
    }
    
    // 不自动显示键盘，等用户点击输入框
    WiFiConfigUpdateStatus("Enter password and tap input field", false);
}

// 隐藏密码输入面板
static void hide_password_panel(void)
{
    if (!password_panel) return;
    
    password_panel_visible = false;
    lv_obj_add_flag(password_panel, LV_OBJ_FLAG_HIDDEN);
    
    if (keyboard) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

// 密码输入事件处理
static void password_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        // 点击输入框时显示键盘
        if (keyboard) {
            lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(keyboard, password_textarea);
        }
    }
    else if (code == LV_EVENT_READY) {
        // 用户按下了确认键
        const char* password = lv_textarea_get_text(password_textarea);
        if (strlen(selected_ssid) > 0) {
            WiFiManager_Connect(selected_ssid, password, true); // 保存配置
            WiFiConfigUpdateStatus("Connecting...", false);
        }
    }
}

// 键盘事件处理
static void keyboard_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        hide_password_panel();
    }
}

// 按钮事件处理
static void btn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_current_target_obj(e);
    
    if (code == LV_EVENT_CLICKED) {
        void * user_data = lv_event_get_user_data(e);
        int btn_id = (int)(uintptr_t)user_data;
        
        switch (btn_id) {
            case 0: // Back
                WiFiConfigHide();
                break;
                
            case 1: // Scan
                WiFiManager_StartScan(wifi_scan_callback);
                if (WiFiManager_GetStatus() == WIFI_STATUS_CONNECTED) {
                    WiFiConfigUpdateStatus("Scanning for new networks...", false);
                } else {
                    WiFiConfigUpdateStatus("Scanning...", false);
                }
                break;
                
            case 2: // Connect (from password panel)
                {
                    const char* password = lv_textarea_get_text(password_textarea);
                    if (strlen(selected_ssid) > 0) {
                        WiFiManager_Connect(selected_ssid, password, true);
                        WiFiConfigUpdateStatus("Connecting...", false);
                    }
                }
                break;
                
            case 3: // Cancel (from password panel)
                hide_password_panel();
                break;
                
            case 4: // Disconnect
                WiFiManager_Disconnect();
                WiFiConfigUpdateStatus("Disconnecting...", false);
                // 刷新连接信息显示
                WiFiConfigRefreshConnection();
                break;
        }
    }
}

extern "C" {

void WiFiConfigCreate(lv_event_cb_t event_cb)
{
    wifi_config_screen = lv_obj_create(NULL);
    
    // 设置深色背景
    lv_obj_set_style_bg_color(wifi_config_screen, lv_color_make(0x1a, 0x1a, 0x1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_config_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // 创建标题
    lv_obj_t * title = lv_label_create(wifi_config_screen);
    lv_label_set_text(title, "WiFi Configuration");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
    
    // 创建按钮容器
    lv_obj_t * btn_cont = lv_obj_create(wifi_config_screen);
    lv_obj_set_size(btn_cont, lv_pct(95), 45);
    lv_obj_align_to(btn_cont, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_cont, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 创建按钮的辅助函数
    auto create_wifi_button = [](lv_obj_t* parent, const char* text, int id) -> lv_obj_t* {
        lv_obj_t * btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 90, 35);
        lv_obj_add_event_cb(btn, btn_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)id);
        
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_center(label);
        
        // 按钮样式
        lv_obj_set_style_bg_color(btn, lv_color_make(0x3a, 0x3a, 0x3a), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(btn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_make(0x5a, 0x5a, 0x5a), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, LV_PART_MAIN);
        
        return btn;
    };
    
    // 创建按钮
    lv_obj_t * back_btn = create_wifi_button(btn_cont, "Back", 0);
    scan_btn = create_wifi_button(btn_cont, "Scan", 1);
    disconnect_btn = create_wifi_button(btn_cont, "Disconnect", 4);
    lv_obj_add_flag(disconnect_btn, LV_OBJ_FLAG_HIDDEN); // 默认隐藏
    
    // 当前连接信息
    lv_obj_t * info_cont = lv_obj_create(wifi_config_screen);
    lv_obj_set_size(info_cont, lv_pct(95), 65);
    lv_obj_align_to(info_cont, btn_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(info_cont, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_border_width(info_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(info_cont, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_radius(info_cont, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_cont, 12, LV_PART_MAIN);
    
    // 连接状态标题
    lv_obj_t * info_title = lv_label_create(info_cont);
    lv_label_set_text(info_title, "Connection Status:");
    lv_obj_set_style_text_color(info_title, lv_color_make(0xaa, 0xaa, 0xaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    current_network_label = lv_label_create(info_cont);
    lv_label_set_text(current_network_label, "Not connected");
    lv_obj_set_style_text_color(current_network_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(current_network_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align_to(current_network_label, info_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    ip_label = lv_label_create(info_cont);
    lv_label_set_text(ip_label, "IP: Not assigned");
    lv_obj_set_style_text_color(ip_label, lv_color_make(0xaa, 0xaa, 0xaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align_to(ip_label, current_network_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);
    
    // 网络列表容器
    lv_obj_t * list_cont = lv_obj_create(wifi_config_screen);
    lv_obj_set_size(list_cont, lv_pct(95), 220);
    lv_obj_align_to(list_cont, info_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(list_cont, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_border_width(list_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(list_cont, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_radius(list_cont, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list_cont, 8, LV_PART_MAIN);
    
    // 列表标题
    lv_obj_t * list_title = lv_label_create(list_cont);
    lv_label_set_text(list_title, "Available Networks:");
    lv_obj_set_style_text_color(list_title, lv_color_make(0xaa, 0xaa, 0xaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(list_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(list_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // 网络列表
    network_list = lv_obj_create(list_cont);
    lv_obj_set_size(network_list, lv_pct(100), lv_pct(85));
    lv_obj_align_to(network_list, list_title, LV_ALIGN_OUT_BOTTOM_LEFT, -8, 5);
    lv_obj_set_style_bg_opa(network_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(network_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(network_list, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(network_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(network_list, 5, LV_PART_MAIN);
    lv_obj_set_scroll_dir(network_list, LV_DIR_VER);
    
    // 状态标签
    status_label = lv_label_create(wifi_config_screen);
    lv_label_set_text(status_label, "Ready to scan networks");
    lv_obj_set_style_text_color(status_label, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // 创建密码输入面板（默认隐藏）
    password_panel = lv_obj_create(wifi_config_screen);
    lv_obj_set_size(password_panel, lv_pct(85), 180);
    lv_obj_center(password_panel);
    lv_obj_set_style_bg_color(password_panel, lv_color_make(0x3a, 0x3a, 0x3a), LV_PART_MAIN);
    lv_obj_set_style_border_width(password_panel, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(password_panel, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN);
    lv_obj_set_style_radius(password_panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(password_panel, 15, LV_PART_MAIN);
    lv_obj_add_flag(password_panel, LV_OBJ_FLAG_HIDDEN);
    
    // 密码面板标题
    lv_obj_t * pwd_title = lv_label_create(password_panel);
    lv_label_set_text(pwd_title, "Enter Password");
    lv_obj_set_style_text_color(pwd_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(pwd_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(pwd_title, LV_ALIGN_TOP_MID, 0, 0);
    
    // 密码输入框
    password_textarea = lv_textarea_create(password_panel);
    lv_obj_set_size(password_textarea, lv_pct(100), 45);
    lv_obj_align_to(password_textarea, pwd_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_textarea_set_placeholder_text(password_textarea, "Enter WiFi password");
    lv_textarea_set_password_mode(password_textarea, true);
    lv_obj_add_event_cb(password_textarea, password_event_handler, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(password_textarea, password_event_handler, LV_EVENT_CLICKED, NULL);
    
    // 密码面板按钮
    lv_obj_t * pwd_btn_cont = lv_obj_create(password_panel);
    lv_obj_set_size(pwd_btn_cont, lv_pct(100), 50);
    lv_obj_align_to(pwd_btn_cont, password_textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_set_style_bg_opa(pwd_btn_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(pwd_btn_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pwd_btn_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(pwd_btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pwd_btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    connect_btn = lv_btn_create(pwd_btn_cont);
    lv_obj_set_size(connect_btn, 110, 40);
    lv_obj_add_event_cb(connect_btn, btn_event_handler, LV_EVENT_CLICKED, (void*)2);
    lv_obj_t * connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_center(connect_label);
    
    lv_obj_t * cancel_btn = lv_btn_create(pwd_btn_cont);
    lv_obj_set_size(cancel_btn, 110, 40);
    lv_obj_add_event_cb(cancel_btn, btn_event_handler, LV_EVENT_CLICKED, (void*)3);
    lv_obj_t * cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    
    // 按钮样式
    lv_obj_set_style_bg_color(connect_btn, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(connect_btn, lv_color_make(0x3A, 0x80, 0xD2), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_make(0x5a, 0x5a, 0x5a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN | LV_STATE_PRESSED);
    
    // 创建键盘（默认隐藏）
    keyboard = lv_keyboard_create(wifi_config_screen);
    lv_obj_set_size(keyboard, lv_pct(100), lv_pct(45));
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_CANCEL, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    
    // 设置WiFi状态回调
    WiFiManager_SetStatusCallback(wifi_status_callback);
    
    // 初始化时刷新连接信息
    WiFiConfigRefreshConnection();
}

void WiFiConfigShow(void)
{
    if (wifi_config_screen) {
        lv_screen_load(wifi_config_screen);
        WiFiConfigRefreshConnection();
        // 自动开始扫描
        WiFiManager_StartScan(wifi_scan_callback);
    }
}

void WiFiConfigHide(void)
{
    if (control_panel_screen) {
        lv_screen_load(control_panel_screen);
    }
}

void WiFiConfigUpdateStatus(const char* status, bool is_error)
{
    if (status_label) {
        lv_label_set_text(status_label, status);
        lv_color_t color = is_error ? lv_color_make(0xff, 0x66, 0x66) : lv_color_make(0x88, 0x88, 0x88);
        lv_obj_set_style_text_color(status_label, color, LV_PART_MAIN);
    }
}

void WiFiConfigUpdateNetworks(void)
{
    WiFiManager_StartScan(wifi_scan_callback);
}

void WiFiConfigRefreshConnection(void)
{
    if (!current_network_label || !ip_label) return;
    
    wifi_status_t status = WiFiManager_GetStatus();
    
    if (status == WIFI_STATUS_CONNECTED) {
        const char* ssid = WiFiManager_GetConnectedSSID();
        int32_t rssi = WiFiManager_GetRSSI();
        IPAddress ip = WiFiManager_GetIP();
        
        char network_text[64];
        snprintf(network_text, sizeof(network_text), "Connected: %s (%ddBm)", ssid, (int)rssi);
        lv_label_set_text(current_network_label, network_text);
        
        char ip_text[32];
        snprintf(ip_text, sizeof(ip_text), "IP: %s", ip.toString().c_str());
        lv_label_set_text(ip_label, ip_text);
        
        lv_obj_set_style_text_color(current_network_label, lv_color_make(0x66, 0xff, 0x66), LV_PART_MAIN);
    } else {
        lv_label_set_text(current_network_label, "Not connected");
        lv_label_set_text(ip_label, "IP: Not assigned");
        lv_obj_set_style_text_color(current_network_label, lv_color_make(0xff, 0x66, 0x66), LV_PART_MAIN);
    }
}

} // extern "C"