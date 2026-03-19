/**
 * @file screens.h
 * @brief LVGL UI屏幕定义
 *
 * 240x240显示屏包含以下屏幕:
 * - 状态屏幕: 显示打印进度、温度、剩余时间
 * - 设置屏幕: WiFi信息、云端状态、重启/重置按钮
 *
 * 布局设计 (适配240x240):
 * - 顶部: Logo区域
 * - 中央: 圆形进度环 + 百分比文字
 * - 底部: 状态信息、温度、剩余时间、作业名
 */

#ifndef SCREENS_H
#define SCREENS_H

#include "lvgl.h"
#include "printer_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 状态屏幕组件
 */
typedef struct {
    lv_obj_t *screen;             // 屏幕对象
    lv_obj_t *logo;              // Logo标签
    lv_obj_t *progress_ring;      // 进度环 (预留)
    lv_obj_t *progress_label;    // 百分比文字
    lv_obj_t *status_label;      // 状态文字
    lv_obj_t *temp_label;        // 温度文字
    lv_obj_t *time_label;        // 剩余时间
    lv_obj_t *job_label;         // 作业名称
    lv_obj_t *wifi_icon;         // WiFi图标 (预留)
    lv_obj_t *cloud_icon;        // 云端图标 (预留)
} status_screen_t;

/**
 * @brief 设置屏幕组件
 */
typedef struct {
    lv_obj_t *screen;             // 屏幕对象
    lv_obj_t *title;             // 标题
    lv_obj_t *ip_label;          // IP地址
    lv_obj_t *ssid_label;        // WiFi SSID
    lv_obj_t *cloud_status_label; // 云端状态
    lv_obj_t *btn_reset;         // 重置配置按钮
    lv_obj_t *btn_reboot;         // 重启按钮
} settings_screen_t;

/**
 * @brief 初始化所有屏幕
 * @return 0 成功
 */
int screens_init(void);

/**
 * @brief 显示状态屏幕
 */
void screens_show_status(void);

/**
 * @brief 显示设置屏幕
 */
void screens_show_settings(void);

/**
 * @brief 更新状态屏幕数据
 * @param state 打印机状态
 */
void screens_update_status(const printer_state_t *state);

/**
 * @brief 更新WiFi状态显示
 * @param connected 是否连接
 * @param ip IP地址
 */
void screens_update_wifi(bool connected, const char *ip);

/**
 * @brief 更新云端状态显示
 * @param status 状态字符串
 */
void screens_update_cloud(const char *status);

/**
 * @brief 全局状态屏幕实例
 */
extern status_screen_t g_status_screen;

/**
 * @brief 全局设置屏幕实例
 */
extern settings_screen_t g_settings_screen;

#ifdef __cplusplus
}
#endif

#endif // SCREENS_H
