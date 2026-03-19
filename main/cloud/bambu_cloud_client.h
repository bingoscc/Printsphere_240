/**
 * @file bambu_cloud_client.h
 * @brief Bambu Lab云端通信接口
 *
 * 功能说明:
 * - 登录认证到 api.bambulab.com
 * - 通过MQTT订阅打印机状态推送
 * - 接收并解析实时打印状态
 *
 * MQTT主题: device/{serial}/report
 */

#ifndef BAMBU_CLOUD_CLIENT_H
#define BAMBU_CLOUD_CLIENT_H

#include "printer_state.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 云端连接状态
 */
typedef enum {
    CLOUD_DISCONNECTED,  // 已断开
    CLOUD_CONNECTING,     // 连接中
    CLOUD_CONNECTED,      // 已连接
    CLOUD_AUTH_FAILED,    // 认证失败
    CLOUD_ERROR          // 错误
} cloud_status_t;

/**
 * @brief 云端状态变更回调
 */
typedef void (*cloud_status_callback_t)(cloud_status_t status, void *user_data);

/**
 * @brief 打印机状态更新回调
 */
typedef void (*cloud_state_callback_t)(const printer_state_t *state, void *user_data);

/**
 * @brief 初始化Bambu Cloud客户端
 * @param username Bambu Lab账号邮箱
 * @param password 账号密码
 * @param device_serial 设备序列号
 * @return ESP_OK 成功
 */
int bambu_cloud_init(const char *username, const char *password, const char *device_serial);

/**
 * @brief 设置回调函数
 * @param status_cb 状态变更回调
 * @param state_cb 状态更新回调
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
int bambu_cloud_set_callbacks(cloud_status_callback_t status_cb,
                              cloud_state_callback_t state_cb,
                              void *user_data);

/**
 * @brief 启动云端连接
 * @return ESP_OK 成功
 */
int bambu_cloud_start(void);

/**
 * @brief 停止云端连接
 * @return ESP_OK 成功
 */
int bambu_cloud_stop(void);

/**
 * @brief 获取当前连接状态
 * @return 云端状态
 */
cloud_status_t bambu_cloud_get_status(void);

/**
 * @brief 获取最新打印机状态
 * @return 打印机状态指针
 */
const printer_state_t* bambu_cloud_get_state(void);

/**
 * @brief 发送命令到打印机
 * @param command 命令名称
 * @param param 命令参数
 * @return ESP_OK 成功
 */
int bambu_cloud_send_command(const char *command, const char *param);

#ifdef __cplusplus
}
#endif

#endif // BAMBU_CLOUD_CLIENT_H
