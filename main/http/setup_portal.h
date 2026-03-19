/**
 * @file setup_portal.h
 * @brief Web配网页面接口
 *
 * 设备首次启动时创建AP热点，用户通过浏览器配置:
 * - WiFi SSID和密码
 * - Bambu Lab账号信息
 * - HomeAssistant MQTT设置
 *
 * Web服务器运行在80端口，提供HTML配置页面
 */

#ifndef SETUP_PORTAL_H
#define SETUP_PORTAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 配网完成回调
 * @param user_data 用户数据
 */
typedef void (*setup_complete_callback_t)(void *user_data);

/**
 * @brief 初始化配网Portal
 * @param ssid AP热点名称
 * @return ESP_OK 成功
 */
int setup_portal_init(const char *ssid);

/**
 * @brief 启动配网Portal
 * @return ESP_OK 成功
 */
int setup_portal_start(void);

/**
 * @brief 停止配网Portal
 * @return ESP_OK 成功
 */
int setup_portal_stop(void);

/**
 * @brief 设置完成回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
int setup_portal_set_complete_callback(setup_complete_callback_t callback, void *user_data);

/**
 * @brief 检查Portal是否运行中
 * @return true 运行中
 */
bool setup_portal_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // SETUP_PORTAL_H
