/**
 * @file hass_mqtt.cpp
 * @brief HomeAssistant MQTT集成实现
 *
 * 实现HASS MQTT发现协议:
 * - 连接成功后自动发布discovery消息
 * - 发布5个传感器: 状态、进度、喷嘴温度、床温、剩余时间
 * - 设备信息注册到HASS实体注册表
 *
 * MQTT主题:
 * - Discovery: homeassistant/sensor/{device_id}/{sensor}/config
 * - State: printsphere/{device_id}/state
 */

#include "hass_mqtt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>

// 日志TAG
static const char *TAG = "HassMqtt";

/**
 * @brief HASS MQTT上下文
 */
typedef struct {
    char mqtt_host[128];              // MQTT服务器地址
    int mqtt_port;                   // MQTT端口
    char mqtt_username[64];           // MQTT用户名
    char mqtt_password[128];         // MQTT密码
    char device_id[64];             // 设备ID
    char device_name[64];           // 设备名称
    bool initialized;               // 是否已初始化
    bool running;                   // 是否正在运行
    bool connected;                 // 是否已连接
    hass_mqtt_connected_callback_t connected_callback;  // 连接回调
    void *user_data;               // 用户数据
    esp_mqtt_client_handle_t mqtt_client;  // MQTT客户端
} hass_mqtt_context_t;

static hass_mqtt_context_t s_ctx = {};

/**
 * @brief 发布HASS发现消息
 *
 * 向HomeAssistant注册以下传感器:
 * - printer_state: 打印机状态
 * - progress: 打印进度
 * - nozzle_temp: 喷嘴温度
 * - bed_temp: 床基温度
 * - remaining_time: 剩余时间
 */
static void hass_publish_discovery(void) {
    char topic[256];
    char payload[512];

    // 打印机状态传感器
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/printer_state/config", s_ctx.device_id);
    snprintf(payload, sizeof(payload),
        "{"
        "\"name\": \"%s 状态\","
        "\"unique_id\": \"%s_printer_state\","
        "\"state_topic\": \"printsphere/%s/state\","
        "\"value_template\": \"{{ value_json.status }}\","
        "\"device\": {\"name\": \"%s\", \"identifiers\": [\"%s\"]}"
        "}",
        s_ctx.device_name, s_ctx.device_id, s_ctx.device_id, s_ctx.device_name, s_ctx.device_id);
    esp_mqtt_client_publish(s_ctx.mqtt_client, topic, payload, 0, 1, true);

    // 打印进度传感器
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/progress/config", s_ctx.device_id);
    snprintf(payload, sizeof(payload),
        "{"
        "\"name\": \"%s 进度\","
        "\"unique_id\": \"%s_progress\","
        "\"state_topic\": \"printsphere/%s/state\","
        "\"value_template\": \"{{ value_json.progress }}\","
        "\"unit_of_measurement\": \"%%\","
        "\"device_class\": \"percentage\","
        "\"device\": {\"name\": \"%s\", \"identifiers\": [\"%s\"]}"
        "}",
        s_ctx.device_name, s_ctx.device_id, s_ctx.device_id, s_ctx.device_name, s_ctx.device_id);
    esp_mqtt_client_publish(s_ctx.mqtt_client, topic, payload, 0, 1, true);

    // 喷嘴温度传感器
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/nozzle_temp/config", s_ctx.device_id);
    snprintf(payload, sizeof(payload),
        "{"
        "\"name\": \"%s 喷嘴温度\","
        "\"unique_id\": \"%s_nozzle_temp\","
        "\"state_topic\": \"printsphere/%s/state\","
        "\"value_template\": \"{{ value_json.nozzle_temp }}\","
        "\"unit_of_measurement\": \"°C\","
        "\"device_class\": \"temperature\","
        "\"device\": {\"name\": \"%s\", \"identifiers\": [\"%s\"]}"
        "}",
        s_ctx.device_name, s_ctx.device_id, s_ctx.device_id, s_ctx.device_name, s_ctx.device_id);
    esp_mqtt_client_publish(s_ctx.mqtt_client, topic, payload, 0, 1, true);

    // 床基温度传感器
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/bed_temp/config", s_ctx.device_id);
    snprintf(payload, sizeof(payload),
        "{"
        "\"name\": \"%s 床温\","
        "\"unique_id\": \"%s_bed_temp\","
        "\"state_topic\": \"printsphere/%s/state\","
        "\"value_template\": \"{{ value_json.bed_temp }}\","
        "\"unit_of_measurement\": \"°C\","
        "\"device_class\": \"temperature\","
        "\"device\": {\"name\": \"%s\", \"identifiers\": [\"%s\"]}"
        "}",
        s_ctx.device_name, s_ctx.device_id, s_ctx.device_id, s_ctx.device_name, s_ctx.device_id);
    esp_mqtt_client_publish(s_ctx.mqtt_client, topic, payload, 0, 1, true);

    // 剩余时间传感器
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/remaining_time/config", s_ctx.device_id);
    snprintf(payload, sizeof(payload),
        "{"
        "\"name\": \"%s 剩余时间\","
        "\"unique_id\": \"%s_remaining_time\","
        "\"state_topic\": \"printsphere/%s/state\","
        "\"value_template\": \"{{ value_json.remaining_time }}\","
        "\"unit_of_measurement\": \"min\","
        "\"device\": {\"name\": \"%s\", \"identifiers\": [\"%s\"]}"
        "}",
        s_ctx.device_name, s_ctx.device_id, s_ctx.device_id, s_ctx.device_name, s_ctx.device_id);
    esp_mqtt_client_publish(s_ctx.mqtt_client, topic, payload, 0, 1, true);

    ESP_LOGI(TAG, "HASS发现消息已发布");
}

/**
 * @brief MQTT事件处理函数
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "HASS MQTT已连接");
            s_ctx.connected = true;

            // 发布发现消息
            hass_publish_discovery();

            if (s_ctx.connected_callback) {
                s_ctx.connected_callback(true, s_ctx.user_data);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HASS MQTT已断开");
            s_ctx.connected = false;
            if (s_ctx.connected_callback) {
                s_ctx.connected_callback(false, s_ctx.user_data);
            }
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "HASS MQTT已订阅");
            break;

        default:
            break;
    }
}

/**
 * @brief 初始化HASS MQTT客户端
 */
int hass_mqtt_init(const char *mqtt_host, int mqtt_port,
                   const char *mqtt_username, const char *mqtt_password,
                   const char *device_id, const char *device_name) {
    memset(&s_ctx, 0, sizeof(s_ctx));

    if (mqtt_host) strncpy(s_ctx.mqtt_host, mqtt_host, sizeof(s_ctx.mqtt_host) - 1);
    s_ctx.mqtt_port = mqtt_port > 0 ? mqtt_port : 1883;
    if (mqtt_username) strncpy(s_ctx.mqtt_username, mqtt_username, sizeof(s_ctx.mqtt_username) - 1);
    if (mqtt_password) strncpy(s_ctx.mqtt_password, mqtt_password, sizeof(s_ctx.mqtt_password) - 1);
    if (device_id) strncpy(s_ctx.device_id, device_id, sizeof(s_ctx.device_id) - 1);
    if (device_name) strncpy(s_ctx.device_name, device_name, sizeof(s_ctx.device_name) - 1);

    // 如果未提供device_id，使用芯片ID生成
    if (strlen(s_ctx.device_id) == 0) {
        snprintf(s_ctx.device_id, sizeof(s_ctx.device_id), "printsphere_%llx", esp_get_chip_id());
    }

    s_ctx.initialized = true;
    ESP_LOGI(TAG, "HASS MQTT初始化: %s:%d, device_id=%s", s_ctx.mqtt_host, s_ctx.mqtt_port, s_ctx.device_id);
    return ESP_OK;
}

/**
 * @brief 设置连接状态回调
 */
int hass_mqtt_set_connected_callback(hass_mqtt_connected_callback_t callback, void *user_data) {
    s_ctx.connected_callback = callback;
    s_ctx.user_data = user_data;
    return ESP_OK;
}

/**
 * @brief 启动MQTT连接
 */
int hass_mqtt_start(void) {
    if (!s_ctx.initialized) {
        ESP_LOGE(TAG, "HASS MQTT未初始化");
        return ESP_FAIL;
    }

    if (s_ctx.running) {
        return ESP_OK;
    }

    char mqtt_uri[256];
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtt://%s:%d", s_ctx.mqtt_host, s_ctx.mqtt_port);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_uri,
        .username = strlen(s_ctx.mqtt_username) > 0 ? s_ctx.mqtt_username : NULL,
        .password = strlen(s_ctx.mqtt_password) > 0 ? s_ctx.mqtt_password : NULL,
        .event_handle = mqtt_event_handler,
        .reconnection_timeout_ms = 10000,
    };

    s_ctx.mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_ctx.mqtt_client) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        return ESP_FAIL;
    }

    esp_mqtt_client_start(s_ctx.mqtt_client);
    s_ctx.running = true;

    ESP_LOGI(TAG, "HASS MQTT已启动");
    return ESP_OK;
}

/**
 * @brief 停止MQTT连接
 */
int hass_mqtt_stop(void) {
    if (!s_ctx.running) {
        return ESP_OK;
    }

    s_ctx.running = false;

    if (s_ctx.mqtt_client) {
        esp_mqtt_client_stop(s_ctx.mqtt_client);
        esp_mqtt_client_destroy(s_ctx.mqtt_client);
        s_ctx.mqtt_client = nullptr;
    }

    ESP_LOGI(TAG, "HASS MQTT已停止");
    return ESP_OK;
}

/**
 * @brief 发布打印机状态到MQTT
 *
 * @param state 打印机状态
 */
int hass_mqtt_publish_state(const printer_state_t *state) {
    if (!s_ctx.connected || !s_ctx.mqtt_client) {
        return ESP_FAIL;
    }

    char topic[128];
    char payload[512];

    snprintf(topic, sizeof(topic), "printsphere/%s/state", s_ctx.device_id);
    snprintf(payload, sizeof(payload),
        "{"
        "\"status\": \"%s\","
        "\"progress\": %.1f,"
        "\"nozzle_temp\": %.1f,"
        "\"nozzle_target\": %.1f,"
        "\"bed_temp\": %.1f,"
        "\"bed_target\": %.1f,"
        "\"remaining_time\": %d,"
        "\"job_name\": \"%s\""
        "}",
        printer_status_to_string(state->status),
        state->progress,
        state->nozzle_temp,
        state->nozzle_target_temp,
        state->bed_temp,
        state->bed_target_temp,
        state->remaining_time / 60,  // 转换为分钟
        state->job_name);

    return esp_mqtt_client_publish(s_ctx.mqtt_client, topic, payload, 0, 1, false) >= 0 ? ESP_OK : ESP_FAIL;
}

/**
 * @brief 检查是否已连接
 */
bool hass_mqtt_is_connected(void) {
    return s_ctx.connected;
}
