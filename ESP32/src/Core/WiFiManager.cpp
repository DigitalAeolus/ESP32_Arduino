#include "WiFiManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 静态变量
static wifi_status_t current_status = WIFI_STATUS_DISCONNECTED;
static wifi_status_callback_t status_callback = NULL;
static wifi_scan_callback_t scan_callback = NULL;
static Preferences preferences;
static wifi_network_t* scan_networks = NULL;
static int scan_network_count = 0;
static unsigned long last_scan_time = 0;
static unsigned long connect_start_time = 0;
static bool is_scanning = false; // 添加扫描状态标志
static wifi_status_t pre_scan_status = WIFI_STATUS_DISCONNECTED; // 扫描前的状态
static const unsigned long CONNECT_TIMEOUT = 15000; // 15秒连接超时
static const unsigned long SCAN_INTERVAL = 5000; // 5秒扫描间隔
static void reset_scan_state();
// 内部函数声明
static void update_status(wifi_status_t status, const char* message);
static void wifi_event_handler(WiFiEvent_t event);

void WiFiManager_Init(void)
{
    Serial.println("WiFiManager: Initializing...");
    
    // 初始化Preferences
    preferences.begin("wifi_config", false);
    
    // 初始化状态变量
    is_scanning = false;
    pre_scan_status = WIFI_STATUS_DISCONNECTED;
    
    // 设置WiFi模式并断开之前的连接
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    
    // 设置WiFi事件处理（在断开连接后设置，避免触发事件）
    WiFi.onEvent(wifi_event_handler);
    
    // 检查实际WiFi状态
    if (WiFi.status() == WL_CONNECTED) {
        current_status = WIFI_STATUS_CONNECTED;
    } else {
        current_status = WIFI_STATUS_DISCONNECTED;
    }
    
    Serial.println("WiFiManager: Initialized");
}

void WiFiManager_StartScan(wifi_scan_callback_t callback)
{
    if (millis() - last_scan_time < SCAN_INTERVAL) {
        return; // 防止频繁扫描
    }
    
    // 如果已经在扫描，先重置状态
    if (is_scanning) {
        Serial.println("WiFiManager: Previous scan still active, resetting...");
        reset_scan_state();
    }
    
    scan_callback = callback;
    
    // 保存扫描前的状态
    pre_scan_status = current_status;
    is_scanning = true;
    
    // 如果当前已连接，不改变连接状态，只更新扫描状态
    if (current_status != WIFI_STATUS_CONNECTED) {
        update_status(WIFI_STATUS_SCANNING, "Scanning for networks...");
    }
    
    // 异步扫描
    WiFi.scanNetworks(true, false, false, 300);
    last_scan_time = millis();
    
    Serial.println("WiFiManager: Started WiFi scan");
}

void WiFiManager_StopScan(void)
{
    WiFi.scanDelete();
    scan_callback = NULL;
    is_scanning = false;
    
    if (current_status == WIFI_STATUS_SCANNING) {
        update_status(WIFI_STATUS_DISCONNECTED, "Scan stopped");
    }
}

// 强制重置扫描状态（用于错误恢复）
static void reset_scan_state(void)
{
    is_scanning = false;
    scan_callback = NULL;
    WiFi.scanDelete();
    Serial.println("WiFiManager: Scan state reset");
}

// 公开的重置扫描状态函数
void WiFiManager_ResetScanState(void)
{
    reset_scan_state();
}

// 调试状态打印
void WiFiManager_PrintStatus(void)
{
    Serial.printf("WiFiManager Status:\n");
    Serial.printf("  Current Status: %s\n", WiFiManager_GetStatusString());
    Serial.printf("  Is Scanning: %s\n", is_scanning ? "Yes" : "No");
    Serial.printf("  WiFi Status: %d\n", WiFi.status());
    Serial.printf("  Pre-scan Status: %d\n", pre_scan_status);
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("  Connected SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
    }
}

void WiFiManager_Connect(const char* ssid, const char* password, bool save_config)
{
    if (!ssid || strlen(ssid) == 0) {
        update_status(WIFI_STATUS_FAILED, "Invalid SSID");
        return;
    }
    
    Serial.printf("WiFiManager: Connecting to %s\n", ssid);
    
    // 如果当前已连接到其他网络，先断开
    if (current_status == WIFI_STATUS_CONNECTED) {
        WiFi.disconnect();
        delay(100); // 短暂延迟确保断开完成
    }
    
    // 保存配置（如果需要）
    if (save_config) {
        WiFiManager_SaveConfig(ssid, password, true);
    }
    
    // 开始连接
    update_status(WIFI_STATUS_CONNECTING, "Connecting...");
    connect_start_time = millis();
    
    if (password && strlen(password) > 0) {
        WiFi.begin(ssid, password);
    } else {
        WiFi.begin(ssid);
    }
}

void WiFiManager_Disconnect(void)
{
    Serial.println("WiFiManager: Disconnecting...");
    WiFi.disconnect(true); // 添加true参数，清除WiFi配置
    WiFi.mode(WIFI_STA); // 重新设置为STA模式
    update_status(WIFI_STATUS_DISCONNECTED, "Disconnected");
}

void WiFiManager_AutoConnect(void)
{
    // 如果正在扫描，不要自动连接
    if (is_scanning) {
        Serial.println("WiFiManager: Skipping auto connect - scan in progress");
        return;
    }
    
    wifi_manager_config_t config;
    if (WiFiManager_LoadConfig(&config) && config.auto_connect) {
        Serial.printf("WiFiManager: Auto connecting to %s\n", config.ssid);
        WiFiManager_Connect(config.ssid, config.password, false);
    } else {
        Serial.println("WiFiManager: No auto connect configuration found");
    }
}

bool WiFiManager_SaveConfig(const char* ssid, const char* password, bool auto_connect)
{
    if (!ssid) return false;
    
    preferences.putString("ssid", ssid);
    preferences.putString("password", password ? password : "");
    preferences.putBool("auto_connect", auto_connect);
    
    Serial.printf("WiFiManager: Config saved - SSID: %s, Auto: %s\n", 
                  ssid, auto_connect ? "Yes" : "No");
    return true;
}

bool WiFiManager_LoadConfig(wifi_manager_config_t* config)
{
    if (!config) return false;
    
    // 检查是否存在SSID键
    if (!preferences.isKey("ssid")) {
        return false;
    }
    
    String ssid = preferences.getString("ssid", "");
    if (ssid.length() == 0) {
        return false;
    }
    
    strncpy(config->ssid, ssid.c_str(), sizeof(config->ssid) - 1);
    config->ssid[sizeof(config->ssid) - 1] = '\0';
    
    String password = preferences.getString("password", "");
    strncpy(config->password, password.c_str(), sizeof(config->password) - 1);
    config->password[sizeof(config->password) - 1] = '\0';
    
    config->auto_connect = preferences.getBool("auto_connect", false);
    
    return true;
}

void WiFiManager_ClearConfig(void)
{
    preferences.clear();
    Serial.println("WiFiManager: Configuration cleared");
}

wifi_status_t WiFiManager_GetStatus(void)
{
    return current_status;
}

const char* WiFiManager_GetStatusString(void)
{
    switch (current_status) {
        case WIFI_STATUS_DISCONNECTED: return "Disconnected";
        case WIFI_STATUS_SCANNING: return "Scanning";
        case WIFI_STATUS_CONNECTING: return "Connecting";
        case WIFI_STATUS_CONNECTED: return "Connected";
        case WIFI_STATUS_FAILED: return "Failed";
        default: return "Unknown";
    }
}

const char* WiFiManager_GetConnectedSSID(void)
{
    if (current_status == WIFI_STATUS_CONNECTED) {
        return WiFi.SSID().c_str();
    }
    return "";
}

int32_t WiFiManager_GetRSSI(void)
{
    if (current_status == WIFI_STATUS_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

IPAddress WiFiManager_GetIP(void)
{
    if (current_status == WIFI_STATUS_CONNECTED) {
        return WiFi.localIP();
    }
    return IPAddress(0, 0, 0, 0);
}

void WiFiManager_SetStatusCallback(wifi_status_callback_t callback)
{
    status_callback = callback;
}

const char* WiFiManager_GetAuthModeString(wifi_auth_mode_t auth_mode)
{
    switch (auth_mode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "Unknown";
    }
}

int WiFiManager_GetSignalStrength(int32_t rssi)
{
    if (rssi >= -50) return 4; // Excellent
    if (rssi >= -60) return 3; // Good
    if (rssi >= -70) return 2; // Fair
    if (rssi >= -80) return 1; // Weak
    return 0; // Very weak
}

// 内部函数实现
static void update_status(wifi_status_t status, const char* message)
{
    current_status = status;
    Serial.printf("WiFiManager: Status changed to %s - %s\n", 
                  WiFiManager_GetStatusString(), message ? message : "");
    
    if (status_callback) {
        status_callback(status, message);
    }
}

static void wifi_event_handler(WiFiEvent_t event)
{
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFiManager: WiFi started");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFiManager: WiFi connected");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("WiFiManager: Got IP: %s\n", WiFi.localIP().toString().c_str());
            update_status(WIFI_STATUS_CONNECTED, "Connected successfully");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFiManager: WiFi disconnected");
            // 如果正在扫描，忽略断开连接事件
            if (is_scanning) {
                Serial.println("WiFiManager: Ignoring disconnect during scan");
                break;
            }
            
            if (current_status == WIFI_STATUS_CONNECTING) {
                update_status(WIFI_STATUS_FAILED, "Connection failed");
            } else {
                update_status(WIFI_STATUS_DISCONNECTED, "Disconnected");
            }
            break;
            
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            {
                int n = WiFi.scanComplete();
                Serial.printf("WiFiManager: Scan completed, found %d networks\n", n);
                
                // 立即重置扫描状态，避免一直忽略断开连接事件
                is_scanning = false;
                
                if (n > 0 && scan_callback) {
                    // 清理之前的扫描结果
                    if (scan_networks) {
                        free(scan_networks);
                    }
                    
                    scan_networks = (wifi_network_t*)malloc(n * sizeof(wifi_network_t));
                    scan_network_count = n;
                    
                    for (int i = 0; i < n; i++) {
                        strncpy(scan_networks[i].ssid, WiFi.SSID(i).c_str(), 
                               sizeof(scan_networks[i].ssid) - 1);
                        scan_networks[i].ssid[sizeof(scan_networks[i].ssid) - 1] = '\0';
                        scan_networks[i].rssi = WiFi.RSSI(i);
                        scan_networks[i].auth_mode = WiFi.encryptionType(i);
                        scan_networks[i].is_hidden = (strlen(scan_networks[i].ssid) == 0);
                    }
                    
                    scan_callback(scan_networks, scan_network_count);
                }
                
                WiFi.scanDelete();
                
                // 扫描完成后，恢复之前的连接状态
                if (current_status == WIFI_STATUS_SCANNING) {
                    if (WiFi.status() == WL_CONNECTED) {
                        update_status(WIFI_STATUS_CONNECTED, "Scan completed - Still connected");
                    } else {
                        update_status(WIFI_STATUS_DISCONNECTED, "Scan completed");
                    }
                } else if (pre_scan_status == WIFI_STATUS_CONNECTED && WiFi.status() == WL_CONNECTED) {
                    // 如果扫描前是连接状态，且现在仍然连接，恢复连接状态
                    update_status(WIFI_STATUS_CONNECTED, "Scan completed - Connection maintained");
                } else {
                    // 确保状态正确
                    if (WiFi.status() == WL_CONNECTED) {
                        update_status(WIFI_STATUS_CONNECTED, "Scan completed - Connected");
                    } else {
                        update_status(WIFI_STATUS_DISCONNECTED, "Scan completed");
                    }
                }
            }
            break;
            
        default:
            break;
    }
}

// 定期检查连接超时和状态恢复
void WiFiManager_Update(void)
{
    static unsigned long last_check = 0;
    
    // 每秒检查一次
    if (millis() - last_check < 1000) {
        return;
    }
    last_check = millis();
    
    // 检查连接超时
    if (current_status == WIFI_STATUS_CONNECTING) {
        if (millis() - connect_start_time > CONNECT_TIMEOUT) {
            Serial.println("WiFiManager: Connection timeout");
            WiFi.disconnect();
            update_status(WIFI_STATUS_FAILED, "Connection timeout");
        }
    }
    
    // 检查扫描超时（如果扫描超过30秒没有完成，强制重置）
    if (is_scanning && (millis() - last_scan_time > 30000)) {
        Serial.println("WiFiManager: Scan timeout, resetting scan state");
        reset_scan_state();
        update_status(WIFI_STATUS_DISCONNECTED, "Scan timeout");
    }
    
    // 状态同步检查
    wl_status_t wifi_status = WiFi.status();
    if (current_status == WIFI_STATUS_CONNECTED && wifi_status != WL_CONNECTED) {
        Serial.println("WiFiManager: Connection lost, updating status");
        update_status(WIFI_STATUS_DISCONNECTED, "Connection lost");
    } else if (current_status == WIFI_STATUS_DISCONNECTED && wifi_status == WL_CONNECTED) {
        Serial.println("WiFiManager: Connection restored, updating status");
        update_status(WIFI_STATUS_CONNECTED, "Connection restored");
    }
}