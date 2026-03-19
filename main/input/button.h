/**
 * @file button.h
 * @brief 单按键输入处理接口
 *
 * genshin_godeye硬件只有1个物理按键
 * 支持多种按键操作:
 * - 短按: 快速按下并释放 (< 长按阈值)
 * - 长按: 按住超过长按阈值
 * - 双击: 两次快速点击
 *
 * 使用GPIO中断+软件消抖实现
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按键事件类型
 */
typedef enum {
    BUTTON_EVENT_PRESS,        // 按下
    BUTTON_EVENT_RELEASE,      // 释放
    BUTTON_EVENT_SHORT_PRESS,  // 短按
    BUTTON_EVENT_LONG_PRESS,   // 长按
    BUTTON_EVENT_DOUBLE_CLICK  // 双击
} button_event_type_t;

/**
 * @brief 按键回调函数类型
 * @param event 事件类型
 * @param hold_time_ms 按住时长(毫秒)
 * @param user_data 用户数据
 */
typedef void (*button_callback_t)(button_event_type_t event, uint32_t hold_time_ms, void *user_data);

/**
 * @brief 按键配置结构体
 */
typedef struct {
    gpio_num_t pin;             // GPIO引脚
    bool active_low;            // 低电平触发(按下为低)
    uint32_t debounce_ms;       // 消抖时间
    uint32_t short_press_ms;    // 短按阈值
    uint32_t long_press_ms;     // 长按阈值
    uint32_t double_click_ms;   // 双击间隔
} button_config_t;

/**
 * @brief 初始化按键
 * @param config 按键配置
 * @return ESP_OK 成功
 */
int button_init(const button_config_t *config);

/**
 * @brief 注册按键回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
int button_register_callback(button_callback_t callback, void *user_data);

/**
 * @brief 检查按键是否被按下
 * @return true 按下，false 未按下
 */
bool button_is_pressed(void);

/**
 * @brief 获取当前按住时长
 * @return 按住时长(毫秒)，未按下返回0
 */
uint32_t button_get_press_time(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_H
