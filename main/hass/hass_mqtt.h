/**
 * @file hass_mqtt.h
 * @brief HomeAssistant MQTT集成接口
 *
 * 实现HomeAssistant MQTT发现协议:
 * - 设备通电后自动发布discovery消息
 * - 设备信息注册到HASS
 * - 状态数据发布到指定主题
 *
 * MQTT主题格式:
 * - Discovery: homeassistant/sensor/{device_id}/{sensor}/config
 * - State: printsphere/{device_id}/state
 */

#ifndef HASS_MQTT_H
#define HASS_MQTT_H

#include "printer_state.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT连接回调
 * @param connected 是否已连接
 * @param user_data 用户数据
 */
typedef void (*hass_mqtt_connected_callback_t)(bool connected, void *user_data);

/**
 * @brief 初始化HASS MQTT客户端
 * @param mqtt_host MQTT服务器地址
 * @param mqtt_port MQTT端口
 * @param mqtt_username MQTT用户名
 * @param mqtt_password MQTT密码
 * @param device_id 设备ID (用于topic)
 * @param device_name 设备名称 (用于HASS显示)
 * @return ESP_OK 成功
 */
int hass_mqtt_init(const char *mqtt_host, int mqtt_port,
                    const char *mqtt_username, const char *mqtt_password,
                    const char *device_id, const char *device_name);

/**
 * @brief 设置连接状态回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
int hass_mqtt_set_connected_callback(hass_mqtt_connected_callback_t callback, void *user_data);

/**
 * @brief 启动MQTT连接
 * @return ESP_OK 成功
 */
int hass_mqtt_start(void);

/**
 * @brief 停止MQTT连接
 * @return ESP_OK 成功
 */
int hass_mqtt_stop(void);

/**
 * @brief 发布打印机状态到MQTT
 * @param state 打印机状态
 * @return ESP_OK 成功
 */
int hass_mqtt_publish_state(const printer_state_t *state);

/**
 * @brief 检查是否已连接
 * @return true 已连接
 */
bool hass_mqtt_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // HASS_MQTT_H
