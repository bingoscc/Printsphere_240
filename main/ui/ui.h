/**
 * @file ui.h
 * @brief UI主接口
 *
 * 协调LVGL显示、TFT驱动、打印机状态显示
 *
 * 屏幕类型:
 * - SCREEN_BOOT: 启动画面
 * - SCREEN_STATUS: 主状态显示
 * - SCREEN_SETTINGS: 设置页面
 * - SCREEN_WIFI_SETUP: WiFi配网
 */

#ifndef UI_H
#define UI_H

#include "printer_state.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 屏幕类型枚举
 */
typedef enum {
    SCREEN_BOOT,        // 启动画面
    SCREEN_STATUS,      // 状态屏幕
    SCREEN_SETTINGS,    // 设置屏幕
    SCREEN_WIFI_SETUP,  // WiFi配网
    SCREEN_COUNT
} ui_screen_t;

/**
 * @brief 初始化UI系统
 * @return ESP_OK 成功
 */
int ui_init(void);

/**
 * @brief 启动UI系统
 * @return ESP_OK 成功
 */
int ui_start(void);

/**
 * @brief 停止UI系统
 * @return ESP_OK 成功
 */
int ui_stop(void);

/**
 * @brief 切换到指定屏幕
 * @param screen 屏幕类型
 */
void ui_set_screen(ui_screen_t screen);

/**
 * @brief 更新打印机状态显示
 * @param state 打印机状态
 */
void ui_update_state(const printer_state_t *state);

/**
 * @brief 更新WiFi状态
 * @param connected 是否连接
 * @param ip IP地址
 */
void ui_set_wifi_state(bool connected, const char *ip);

/**
 * @brief 更新云端连接状态
 * @param status 状态字符串
 */
void ui_set_cloud_state(const char *status);

/**
 * @brief 刷新UI
 */
void ui_invalidate(void);

#ifdef __cplusplus
}
#endif

#endif // UI_H
