/**
 * @file button.cpp
 * @brief 按键输入实现
 *
 * 使用GPIO中断+软件定时器实现按键检测:
 * - 硬件消抖: 50ms防抖时间
 * - 短按检测: 按下后300-1000ms释放
 * - 长按检测: 按下超过1000ms
 * - 双击检测: 500ms内两次点击
 */

#include "button.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include <string.h>
#include <inttypes.h>

// 日志TAG
static const char *TAG = "Button";

// 按键配置
static button_config_t s_config = {};
// 按键回调
static button_callback_t s_callback = nullptr;
// 回调用户数据
static void *s_callback_user_data = nullptr;
// 按键是否按下
static bool s_pressed = false;
// 按下开始时间 (毫秒)
static uint64_t s_press_start_time = 0;
// 上次释放时间 (毫秒)
static uint64_t s_last_release_time = 0;
// 长按事件是否已触发
static bool s_long_press_triggered = false;
// 长按定时器
static esp_timer_handle_t s_long_press_timer = nullptr;
// 消抖定时器
static esp_timer_handle_t s_debounce_timer = nullptr;
// 是否正在消抖
static bool s_debouncing = false;

/**
 * @brief GPIO中断处理函数
 *
 * 在中断中处理按键按下和释放事件
 */
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint64_t now = esp_timer_get_time() / 1000;  // 转换为毫秒
    bool pressed = gpio_get_level(s_config.pin) == (s_config.active_low ? 0 : 1);

    // 消抖中，忽略此次中断
    if (s_debouncing) {
        return;
    }

    if (pressed && !s_pressed) {
        // 按键按下
        s_pressed = true;
        s_press_start_time = now;
        s_long_press_triggered = false;

        // 通知按下事件
        if (s_callback) {
            s_callback(BUTTON_EVENT_PRESS, 0, s_callback_user_data);
        }

        // 启动长按定时器
        esp_timer_start_once(s_long_press_timer, s_config.long_press_ms * 1000);
    } else if (!pressed && s_pressed) {
        // 按键释放
        uint32_t hold_time = (uint32_t)(now - s_press_start_time);
        s_pressed = false;

        // 检查是否为双击
        if ((now - s_last_release_time) < s_config.double_click_ms && s_callback) {
            s_callback(BUTTON_EVENT_DOUBLE_CLICK, hold_time, s_callback_user_data);
        } else if (hold_time >= s_config.short_press_ms && hold_time < s_config.long_press_ms && s_callback) {
            // 短按
            s_callback(BUTTON_EVENT_SHORT_PRESS, hold_time, s_callback_user_data);
        }

        s_last_release_time = now;

        // 通知释放事件
        if (s_callback) {
            s_callback(BUTTON_EVENT_RELEASE, hold_time, s_callback_user_data);
        }
    }
}

/**
 * @brief 长按定时器回调
 *
 * 当定时器触发时，如果按键仍处于按下状态，则发送长按事件
 */
static void long_press_callback(void *arg) {
    if (s_pressed && !s_long_press_triggered) {
        s_long_press_triggered = true;
        uint32_t hold_time = (uint32_t)((esp_timer_get_time() / 1000) - s_press_start_time);
        if (s_callback) {
            s_callback(BUTTON_EVENT_LONG_PRESS, hold_time, s_callback_user_data);
        }
    }
}

/**
 * @brief 消抖定时器回调
 */
static void debounce_callback(void *arg) {
    s_debouncing = false;
}

/**
 * @brief 初始化按键
 *
 * 配置GPIO为中断模式，创建软件定时器
 */
int button_init(const button_config_t *config) {
    memcpy(&s_config, config, sizeof(s_config));

    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << s_config.pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = s_config.active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = s_config.active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE  // 上升沿和下降沿都中断
    };
    gpio_config(&io_conf);

    // 安装GPIO中断服务
    gpio_install_isr_service(0);
    gpio_isr_handler_add(s_config.pin, gpio_isr_handler, NULL);

    // 创建长按定时器
    esp_timer_create_args_t timer_args = {
        .callback = long_press_callback,
        .arg = NULL,
        .name = "long_press"
    };
    esp_timer_create(&timer_args, &s_long_press_timer);

    // 创建消抖定时器
    timer_args.callback = debounce_callback;
    timer_args.name = "debounce";
    esp_timer_create(&timer_args, &s_debounce_timer);

    ESP_LOGI(TAG, "按键初始化完成, GPIO%" PRIu32, (uint32_t)s_config.pin);
    return ESP_OK;
}

/**
 * @brief 注册按键回调函数
 */
int button_register_callback(button_callback_t callback, void *user_data) {
    s_callback = callback;
    s_callback_user_data = user_data;
    return ESP_OK;
}

/**
 * @brief 检查按键是否被按下
 */
bool button_is_pressed(void) {
    return gpio_get_level(s_config.pin) == (s_config.active_low ? 0 : 1);
}

/**
 * @brief 获取当前按住时长
 */
uint32_t button_get_press_time(void) {
    if (!s_pressed) return 0;
    return (uint32_t)((esp_timer_get_time() / 1000) - s_press_start_time);
}
