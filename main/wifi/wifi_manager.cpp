/**
 * @file wifi_manager.cpp
 * @brief WiFi管理实现
 *
 * 支持AP+STA双模式同时运行:
 * - STA模式: 连接WiFi路由器上网
 * - AP模式: 创建热点供设备配网
 *
 * 使用事件回调机制通知连接状态变化
 */

#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

// 日志TAG
static const char *TAG = "WiFiManager";

// 当前WiFi状态
static wifi_state_t s_wifi_state = WIFI_STATE_IDLE;
// 设备IP地址
static char s_device_ip[16] = "0.0.0.0";
// 状态变更回调函数
static wifi_state_callback_t s_callback = nullptr;
// 回调用户数据
static void *s_callback_user_data = nullptr;

/**
 * @brief WiFi事件处理函数
 *
 * 捕获WiFi和IP事件，更新状态并通知回调
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                // STA模式启动成功，开始连接
                ESP_LOGI(TAG, "WiFi STA已启动");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED:
                // 已连接到AP，等待获取IP
                ESP_LOGI(TAG, "WiFi已连接到AP");
                s_wifi_state = WIFI_STATE_CONNECTING;
                if (s_callback) {
                    s_callback(s_wifi_state, s_callback_user_data);
                }
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                // WiFi断开连接
                ESP_LOGI(TAG, "WiFi已断开");
                s_wifi_state = WIFI_STATE_DISCONNECTED;
                if (s_callback) {
                    s_callback(s_wifi_state, s_callback_user_data);
                }
                break;

            case WIFI_EVENT_AP_START:
                // AP模式启动成功
                ESP_LOGI(TAG, "WiFi AP已启动");
                s_wifi_state = WIFI_STATE_AP_RUNNING;
                if (s_callback) {
                    s_callback(s_wifi_state, s_callback_user_data);
                }
                break;

            case WIFI_EVENT_AP_STOP:
                // AP模式停止
                ESP_LOGI(TAG, "WiFi AP已停止");
                break;

            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            // 获取到IP地址，连接成功
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            snprintf(s_device_ip, sizeof(s_device_ip), IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "获取到IP: %s", s_device_ip);
            s_wifi_state = WIFI_STATE_CONNECTED;
            if (s_callback) {
                s_callback(s_wifi_state, s_callback_user_data);
            }
        }
    }
}

/**
 * @brief 初始化WiFi管理器
 *
 * 配置AP+STA双模式，注册事件处理器
 */
int wifi_manager_init(void) {
    ESP_LOGI(TAG, "初始化WiFi");

    // 初始化TCP/IP协议栈
    ESP_ERROR_CHECK(esp_netif_init());

    // 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建WiFi STA接口
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // 创建WiFi AP接口
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    // WiFi初始化配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册WiFi事件处理器
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    // 注册IP事件处理器
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // 设置为AP+STA双模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_LOGI(TAG, "WiFi管理器初始化完成");
    return ESP_OK;
}

/**
 * @brief 连接WiFi网络 (STA模式)
 *
 * @param ssid WiFi名称
 * @param password WiFi密码
 */
int wifi_manager_connect(const char *ssid, const char *password) {
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

    // 如果有密码则设置密码和加密方式
    if (password && strlen(password) > 0) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = password && strlen(password) > 0 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_state = WIFI_STATE_CONNECTING;
    ESP_LOGI(TAG, "正在连接WiFi: %s", ssid);
    return ESP_OK;
}

/**
 * @brief 断开WiFi连接
 */
int wifi_manager_disconnect(void) {
    esp_wifi_disconnect();
    s_wifi_state = WIFI_STATE_DISCONNECTED;
    return ESP_OK;
}

/**
 * @brief 启动AP模式 (热点)
 *
 * @param ssid 热点名称
 * @param password 热点密码 (空字符串表示无密码)
 */
int wifi_manager_start_ap(const char *ssid, const char *password) {
    wifi_config_t ap_config = {};
    strncpy((char *)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid) - 1);
    ap_config.ap.ssid_len = strlen(ssid);

    // 设置AP密码和加密方式
    if (password && strlen(password) > 0) {
        strncpy((char *)ap_config.ap.password, password, sizeof(ap_config.ap.password) - 1);
        ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    } else {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ap_config.ap.max_connection = 4;      // 最大连接数4个
    ap_config.ap.beacon_interval = 100;   // 信标间隔100ms

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "启动AP: %s", ssid);
    return ESP_OK;
}

/**
 * @brief 停止AP模式
 */
int wifi_manager_stop_ap(void) {
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    return ESP_OK;
}

/**
 * @brief 获取当前WiFi状态
 */
wifi_state_t wifi_manager_get_state(void) {
    return s_wifi_state;
}

/**
 * @brief 检查是否已连接到WiFi
 */
bool wifi_manager_is_connected(void) {
    return s_wifi_state == WIFI_STATE_CONNECTED;
}

/**
 * @brief 注册WiFi状态回调函数
 *
 * @param callback 回调函数
 * @param user_data 用户数据
 */
int wifi_manager_register_callback(wifi_state_callback_t callback, void *user_data) {
    s_callback = callback;
    s_callback_user_data = user_data;
    return ESP_OK;
}

/**
 * @brief 获取设备IP地址
 */
const char* wifi_manager_get_ip(void) {
    return s_device_ip;
}
