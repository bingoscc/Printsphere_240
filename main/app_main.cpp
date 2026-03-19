/**
 * @file app_main.cpp
 * @brief PrintSphere 240 主程序入口
 *
 * PrintSphere - Bambu Lab 打印机伴侣
 * 适配硬件: genshin_godeye (ESP32-S3 + 240x240 SPI TFT)
 *
 * 功能说明:
 * - WiFi Manager: AP+STA 双模，支持配网
 * - Bambu Cloud: 云端MQTT通信，实时打印机状态
 * - HomeAssistant: MQTT发现协议，自动集成
 * - LVGL UI: 240x240 TFT显示，支持进度环、温度等信息
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "config_store.h"
#include "button.h"
#include "ui.h"
#include "bambu_cloud_client.h"
#include "printer_client.h"
#include "hass_mqtt.h"
#include "setup_portal.h"
#include "board_pins.h"

// 日志TAG
static const char *TAG = "PrintSphere";

/**
 * @brief 连接状态结构体
 * 跟踪云服务连接状态
 */
typedef struct {
    bool cloud_connected;    // Bambu Cloud连接状态
    bool printer_connected;  // 本地打印机连接状态
    bool hass_connected;    // HomeAssistant MQTT连接状态
} connection_status_t;

static connection_status_t s_conn_status = {};

/**
 * @brief WiFi状态回调函数
 * 当WiFi连接/断开时调用，更新UI并启动/停止相关服务
 */
static void wifi_state_callback(wifi_state_t state, void *user_data) {
    ESP_LOGI(TAG, "WiFi状态改变: %d", state);

    switch (state) {
        case WIFI_STATE_CONNECTED:
            ESP_LOGI(TAG, "WiFi已连接");
            ui_set_wifi_state(true, wifi_manager_get_ip());

            // WiFi连接后启动云服务
            if (s_conn_status.cloud_connected) {
                bambu_cloud_start();
            }
            if (s_conn_status.hass_connected) {
                hass_mqtt_start();
            }
            break;

        case WIFI_STATE_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi已断开");
            ui_set_wifi_state(false, NULL);
            break;

        case WIFI_STATE_AP_RUNNING:
            ESP_LOGI(TAG, "AP模式运行中");
            break;

        default:
            break;
    }
}

/**
 * @brief Bambu Cloud状态回调
 * 处理云端连接/断开/认证失败等状态变化
 */
static void cloud_status_callback(cloud_status_t status, void *user_data) {
    switch (status) {
        case CLOUD_CONNECTED:
            ESP_LOGI(TAG, "Bambu Cloud已连接");
            ui_set_cloud_state("已连接");
            s_conn_status.cloud_connected = true;
            break;
        case CLOUD_DISCONNECTED:
            ui_set_cloud_state("已断开");
            s_conn_status.cloud_connected = false;
            break;
        case CLOUD_AUTH_FAILED:
            ui_set_cloud_state("认证失败");
            break;
        case CLOUD_ERROR:
            ui_set_cloud_state("错误");
            break;
        default:
            break;
    }
}

/**
 * @brief 云端打印机状态回调
 * 收到云端推送的打印机状态时调用，更新UI并转发到HASS
 */
static void cloud_state_callback(const printer_state_t *state, void *user_data) {
    if (state) {
        ui_update_state(state);

        // 转发到HomeAssistant
        if (s_conn_status.hass_connected) {
            hass_mqtt_publish_state(state);
        }
    }
}

/**
 * @brief 本地打印机状态回调
 * 本地打印机HTTP轮询返回状态时调用
 */
static void printer_state_callback(const printer_state_t *state, void *user_data) {
    if (state) {
        ui_update_state(state);

        if (s_conn_status.hass_connected) {
            hass_mqtt_publish_state(state);
        }
    }
}

/**
 * @brief 按键事件回调
 *
 * 按键操作映射:
 * - 短按(300-1000ms): 切换视图
 * - 长按(>1000ms): 进入设置页面
 * - 双击: 重启设备
 */
static void button_callback(button_event_type_t event, uint32_t hold_time_ms, void *user_data) {
    ESP_LOGI(TAG, "按键事件: %d, 按住时间: %ums", event, hold_time_ms);

    switch (event) {
        case BUTTON_EVENT_SHORT_PRESS:
            // 短按 - 切换视图（可扩展）
            break;

        case BUTTON_EVENT_LONG_PRESS:
            // 长按 - 进入设置页面
            ui_set_screen(SCREEN_SETTINGS);
            break;

        case BUTTON_EVENT_DOUBLE_CLICK:
            // 双击 - 重启设备
            ESP_LOGI(TAG, "双击 - 正在重启...");
            esp_restart();
            break;

        default:
            break;
    }
}

/**
 * @brief HomeAssistant MQTT连接回调
 */
static void hass_connected_callback(bool connected, void *user_data) {
    s_conn_status.hass_connected = connected;
    ESP_LOGI(TAG, "HASS MQTT %s", connected ? "已连接" : "已断开");
}

/**
 * @brief 配网页面完成回调
 * 用户完成配网后调用，停止AP和HTTP服务器
 */
static void setup_complete_callback(void *user_data) {
    ESP_LOGI(TAG, "配网完成，停止Portal");
    setup_portal_stop();
}

/**
 * @brief 应用主入口
 */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "PrintSphere 240 启动中...");

    // 1. 初始化NVS存储
    config_store_init();

    // 2. 初始化WiFi（AP+STA模式）
    wifi_manager_init();
    wifi_manager_register_callback(wifi_state_callback, NULL);

    // 3. 初始化按键（GPIO38，低电平触发）
    button_config_t btn_config = {
        .pin = PIN_BTN,
        .active_low = true,         // 按键按下时为低电平
        .debounce_ms = 50,           // 防抖时间50ms
        .short_press_ms = 300,        // 短按阈值300ms
        .long_press_ms = 1000,       // 长按阈值1000ms
        .double_click_ms = 500       // 双击间隔500ms
    };
    button_init(&btn_config);
    button_register_callback(button_callback, NULL);

    // 4. 加载配置
    printsphere_config_t config;
    config_store_get(&config);

    // 5. 初始化UI
    ui_init();
    ui_start();

    // 6. 检查是否已配网
    if (!config_store_is_provisioned()) {
        ESP_LOGI(TAG, "设备未配网，启动Setup Portal");

        // 启动AP供手机连接配网
        wifi_manager_start_ap("PrintSphere-Setup", "");

        // 启动Web配网页面
        setup_portal_init("PrintSphere-Setup");
        setup_portal_set_complete_callback(setup_complete_callback, NULL);
        setup_portal_start();

        // 停留在配网模式
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 7. 已配网，连接WiFi
    ESP_LOGI(TAG, "正在连接WiFi: %s", config.wifi_ssid);
    wifi_manager_connect(config.wifi_ssid, config.wifi_password);

    // 等待WiFi连接成功
    while (!wifi_manager_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // 8. 初始化并启动Bambu Cloud服务
    if (config.cloud_enabled) {
        bambu_cloud_init(config.bambu_username, config.bambu_password, config.bambu_device_serial);
        bambu_cloud_set_callbacks(cloud_status_callback, cloud_state_callback, NULL);
        bambu_cloud_start();
    }

    // 9. 初始化并启动HomeAssistant MQTT服务
    if (config.hass_enabled) {
        hass_mqtt_init(config.mqtt_host, config.mqtt_port,
                       config.mqtt_username, config.mqtt_password,
                       "printsphere", "PrintSphere");
        hass_mqtt_set_connected_callback(hass_connected_callback, NULL);
        hass_mqtt_start();
    }

    // 10. 主循环 - 处理LVGL任务
    ESP_LOGI(TAG, "进入主循环");
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
