/**
 * @file printer_client.h
 * @brief 本地打印机通信接口
 *
 * 通过HTTP API轮询本地打印机状态
 * 适用于与打印机在同一局域网的场景
 *
 * API端点: http://{host}:{port}/api/printer/status
 */

#ifndef PRINTER_CLIENT_H
#define PRINTER_CLIENT_H

#include "printer_state.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 打印机状态回调
 * @param state 打印机状态
 * @param user_data 用户数据
 */
typedef void (*printer_client_callback_t)(const printer_state_t *state, void *user_data);

/**
 * @brief 初始化打印机客户端
 * @param host 打印机IP地址
 * @param port 打印机HTTP端口 (默认7125)
 * @return ESP_OK 成功
 */
int printer_client_init(const char *host, int port);

/**
 * @brief 设置状态回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
int printer_client_set_callback(printer_client_callback_t callback, void *user_data);

/**
 * @brief 启动轮询
 * @return ESP_OK 成功
 */
int printer_client_start(void);

/**
 * @brief 停止轮询
 * @return ESP_OK 成功
 */
int printer_client_stop(void);

/**
 * @brief 检查是否已连接
 * @return true 已连接
 */
bool printer_client_is_connected(void);

/**
 * @brief 获取最新状态
 * @return 打印机状态指针
 */
const printer_state_t* printer_client_get_state(void);

/**
 * @brief 发送命令到打印机
 * @param cmd JSON格式命令
 * @return ESP_OK 成功
 */
int printer_client_send_command(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif // PRINTER_CLIENT_H
