/**
 * @file wifi_manager.h
 * @brief WiFi管理接口
 *
 * 支持AP+STA双模式:
 * - STA模式: 连接到路由器
 * - AP模式: 创建热点供设备配网
 *
 * 使用事件回调机制通知WiFi状态变化
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi状态枚举
 */
typedef enum {
    WIFI_STATE_IDLE,           // 空闲
    WIFI_STATE_CONNECTING,     // 连接中
    WIFI_STATE_CONNECTED,      // 已连接
    WIFI_STATE_DISCONNECTED,   // 已断开
    WIFI_STATE_AP_RUNNING      // AP模式运行中
} wifi_state_t;

/**
 * @brief WiFi状态回调函数类型
 * @param state 新的WiFi状态
 * @param user_data 用户数据指针
 */
typedef void (*wifi_state_callback_t)(wifi_state_t state, void *user_data);

/**
 * @brief 初始化WiFi管理器
 * @return ESP_OK 成功
 */
int wifi_manager_init(void);

/**
 * @brief 连接到指定WiFi网络 (STA模式)
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @return ESP_OK 成功
 */
int wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief 断开WiFi连接
 * @return ESP_OK 成功
 */
int wifi_manager_disconnect(void);

/**
 * @brief 启动AP模式 (热点)
 * @param ssid AP热点名称
 * @param password AP密码 (可为空字符串表示无密码)
 * @return ESP_OK 成功
 */
int wifi_manager_start_ap(const char *ssid, const char *password);

/**
 * @brief 停止AP模式
 * @return ESP_OK 成功
 */
int wifi_manager_stop_ap(void);

/**
 * @brief 获取当前WiFi状态
 * @return WiFi状态
 */
wifi_state_t wifi_manager_get_state(void);

/**
 * @brief 检查是否已连接到WiFi
 * @return true 已连接，false 未连接
 */
bool wifi_manager_is_connected(void);

/**
 * @brief 注册WiFi状态回调函数
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
int wifi_manager_register_callback(wifi_state_callback_t callback, void *user_data);

/**
 * @brief 获取设备IP地址
 * @return IP地址字符串
 */
const char* wifi_manager_get_ip(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
