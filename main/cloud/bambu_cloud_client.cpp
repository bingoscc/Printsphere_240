/**
 * @file bambu_cloud_client.cpp
 * @brief Bambu Lab云端通信实现
 *
 * 功能流程:
 * 1. 登录认证到 api.bambulab.com 获取access_token
 * 2. 使用token连接 MQTT (mqtt.bambulab.com:8883)
 * 3. 订阅主题 device/{serial}/report 接收打印机状态推送
 * 4. 解析JSON状态并通过回调通知
 *
 * 断线后自动重连 (5秒间隔)
 */

#include "bambu_cloud_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>

// 日志TAG
static const char *TAG = "BambuCloud";

// Bambu Lab API服务器
static const char *BAMBU_API_HOST = "api.bambulab.com";
// Bambu Lab MQTT服务器
static const char *BAMBU_MQTT_HOST = "mqtt.bambulab.com";
static const int BAMBU_MQTT_PORT = 8883;

/**
 * @brief 云端客户端上下文
 */
typedef struct {
    char username[128];              // Bambu账号邮箱
    char password[128];              // 账号密码
    char device_serial[32];         // 设备序列号
    char access_token[256];         // 访问令牌
    char token_type[32];            // 令牌类型
    char mqtt_username[64];          // MQTT用户名
    char mqtt_password[128];        // MQTT密码
    bool initialized;               // 是否已初始化
    bool running;                  // 是否正在运行
    cloud_status_t status;          // 当前状态
    printer_state_t printer_state;  // 打印机状态
    cloud_status_callback_t status_callback;   // 状态回调
    cloud_state_callback_t state_callback;     // 状态更新回调
    void *user_data;               // 用户数据
    esp_mqtt_client_handle_t mqtt_client;     // MQTT客户端句柄
    esp_timer_handle_t reconnect_timer;       // 重连定时器
} bambu_cloud_context_t;

static bambu_cloud_context_t s_ctx = {};

static void reconnect_timer_callback(void *arg);

/**
 * @brief 发送HTTP请求到Bambu API
 *
 * @param method HTTP方法 (GET/POST)
 * @param path API路径
 * @param body 请求体JSON
 * @param response 响应缓冲区
 * @param resp_len 缓冲区长度
 */
static int cloud_http_request(const char *method, const char *path, const char *body,
                               char *response, size_t resp_len) {
    char url[512];
    snprintf(url, sizeof(url), "https://%s%s", BAMBU_API_HOST, path);

    esp_http_client_config_t config = {
        .url = url,
        .method = method,
        .timeout_ms = 10000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    // 添加认证头
    if (strlen(s_ctx.access_token) > 0) {
        char auth_header[384];
        snprintf(auth_header, sizeof(auth_header), "%s %s", s_ctx.token_type, s_ctx.access_token);
        esp_http_client_set_header(client, "Authorization", auth_header);
    }

    if (body && strlen(body) > 0) {
        esp_http_client_set_post_fields(client, body, strlen(body));
    }

    int err = esp_http_client_open(client, strlen(body) > 0 ? strlen(body) : 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int read_len = esp_http_client_read(client, response, resp_len - 1);
    if (read_len > 0) {
        response[read_len] = '\0';
    }

    esp_http_client_cleanup(client);
    return ESP_OK;
}

/**
 * @brief 登录Bambu Cloud
 *
 * 发送用户名密码到认证端点，获取access_token
 */
static int bamb_cloud_login(void) {
    ESP_LOGI(TAG, "正在登录Bambu Cloud...");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "username", s_ctx.username);
    cJSON_AddStringToObject(root, "password", s_ctx.password);
    cJSON_AddStringToObject(root, "grant_type", "password");

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    char response[2048] = {};
    int ret = cloud_http_request("POST", "/v1/auth/login", json_str, response, sizeof(response));
    free(json_str);
    cJSON_Delete(root);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "登录请求失败");
        return ret;
    }

    cJSON *resp = cJSON_Parse(response);
    if (!resp) {
        ESP_LOGE(TAG, "解析登录响应失败");
        return ESP_FAIL;
    }

    cJSON *access_token = cJSON_GetObjectItem(resp, "access_token");
    cJSON *token_type = cJSON_GetObjectItem(resp, "token_type");

    if (access_token && token_type) {
        strncpy(s_ctx.access_token, access_token->valuestring, sizeof(s_ctx.access_token) - 1);
        strncpy(s_ctx.token_type, token_type->valuestring, sizeof(s_ctx.token_type) - 1);
        ESP_LOGI(TAG, "登录成功");
    } else {
        cJSON_Delete(resp);
        return ESP_FAIL;
    }

    cJSON_Delete(resp);
    return ESP_OK;
}

/**
 * @brief MQTT事件处理函数
 *
 * 处理连接/断开/消息接收事件
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT已连接");
            s_ctx.status = CLOUD_CONNECTED;

            // 订阅设备状态主题
            char topic[128];
            snprintf(topic, sizeof(topic), "device/%s/report", s_ctx.device_serial);
            esp_mqtt_client_subscribe(event->client, topic, 1);

            if (s_ctx.status_callback) {
                s_ctx.status_callback(s_ctx.status, s_ctx.user_data);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT已断开");
            s_ctx.status = CLOUD_DISCONNECTED;
            if (s_ctx.status_callback) {
                s_ctx.status_callback(s_ctx.status, s_ctx.user_data);
            }

            // 调度重连 (5秒后)
            esp_timer_start_once(s_ctx.reconnect_timer, 5000000);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "收到MQTT数据: topic=%.*s", event->topic_len, event->topic);

            // 解析设备状态JSON
            if (strncmp(event->topic, "device/", 7) == 0 &&
                strstr(event->topic, "/report") != NULL) {
                cJSON *root = cJSON_Parse(event->data);
                if (root) {
                    // 解析打印状态
                    cJSON *print_status = cJSON_GetObjectItem(root, "print_status");
                    if (print_status) {
                        if (strcmp(print_status->valuestring, "printing") == 0) {
                            s_ctx.printer_state.status = PRINTER_PRINTING;
                        } else if (strcmp(print_status->valuestring, "idle") == 0) {
                            s_ctx.printer_state.status = PRINTER_IDLE;
                        } else if (strcmp(print_status->valuestring, "paused") == 0) {
                            s_ctx.printer_state.status = PRINTER_PAUSED;
                        }
                    }

                    // 解析进度
                    cJSON *progress = cJSON_GetObjectItem(root, "progress");
                    if (progress) {
                        s_ctx.printer_state.progress = (float)progress->valueint;
                    }

                    // 解析作业名称
                    cJSON *gcode_state = cJSON_GetObjectItem(root, "gcode_state");
                    if (gcode_state) {
                        strncpy(s_ctx.printer_state.job_name, gcode_state->valuestring,
                                sizeof(s_ctx.printer_state.job_name) - 1);
                    }

                    // 通知状态更新
                    if (s_ctx.state_callback) {
                        s_ctx.state_callback(&s_ctx.printer_state, s_ctx.user_data);
                    }

                    cJSON_Delete(root);
                }
            }
            break;

        default:
            break;
    }
}

/**
 * @brief 重连定时器回调
 */
static void reconnect_timer_callback(void *arg) {
    if (s_ctx.running && s_ctx.status == CLOUD_DISCONNECTED) {
        ESP_LOGI(TAG, "正在尝试重连...");
        bambu_cloud_start();
    }
}

/**
 * @brief 初始化Bambu Cloud客户端
 */
int bambu_cloud_init(const char *username, const char *password, const char *device_serial) {
    memset(&s_ctx, 0, sizeof(s_ctx));

    if (username) strncpy(s_ctx.username, username, sizeof(s_ctx.username) - 1);
    if (password) strncpy(s_ctx.password, password, sizeof(s_ctx.password) - 1);
    if (device_serial) strncpy(s_ctx.device_serial, device_serial, sizeof(s_ctx.device_serial) - 1);

    s_ctx.initialized = true;
    printer_state_init(&s_ctx.printer_state);

    // 创建重连定时器
    esp_timer_create_args_t timer_args = {
        .callback = reconnect_timer_callback,
        .arg = &s_ctx,
        .name = "cloud_reconnect"
    };
    esp_timer_create(&timer_args, &s_ctx.reconnect_timer);

    ESP_LOGI(TAG, "Bambu Cloud客户端初始化完成");
    return ESP_OK;
}

/**
 * @brief 设置回调函数
 */
int bambu_cloud_set_callbacks(cloud_status_callback_t status_cb,
                               cloud_state_callback_t state_cb,
                               void *user_data) {
    s_ctx.status_callback = status_cb;
    s_ctx.state_callback = state_cb;
    s_ctx.user_data = user_data;
    return ESP_OK;
}

/**
 * @brief 启动云端连接
 */
int bambu_cloud_start(void) {
    if (!s_ctx.initialized) {
        ESP_LOGE(TAG, "云端客户端未初始化");
        return ESP_FAIL;
    }

    if (s_ctx.running) {
        ESP_LOGW(TAG, "云端客户端已在运行");
        return ESP_OK;
    }

    s_ctx.running = true;
    s_ctx.status = CLOUD_CONNECTING;

    // 需要先登录获取token
    if (strlen(s_ctx.access_token) == 0) {
        if (bamb_cloud_login() != ESP_OK) {
            s_ctx.status = CLOUD_AUTH_FAILED;
            return ESP_FAIL;
        }
    }

    // 配置MQTT客户端
    char mqtt_uri[256];
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtt://%s:%d", BAMBU_MQTT_HOST, BAMBU_MQTT_PORT);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_uri,
        .username = s_ctx.mqtt_username,
        .password = s_ctx.mqtt_password,
        .event_handle = mqtt_event_handler,
        .transport = MQTT_TRANSPORT_OVER_SSL,
        .reconnection_timeout_ms = 10000,
    };

    s_ctx.mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_ctx.mqtt_client) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        return ESP_FAIL;
    }

    esp_mqtt_client_start(s_ctx.mqtt_client);

    if (s_ctx.status_callback) {
        s_ctx.status_callback(s_ctx.status, s_ctx.user_data);
    }

    ESP_LOGI(TAG, "Bambu Cloud客户端已启动");
    return ESP_OK;
}

/**
 * @brief 停止云端连接
 */
int bambu_cloud_stop(void) {
    if (!s_ctx.running) {
        return ESP_OK;
    }

    s_ctx.running = false;

    if (s_ctx.mqtt_client) {
        esp_mqtt_client_stop(s_ctx.mqtt_client);
        esp_mqtt_client_destroy(s_ctx.mqtt_client);
        s_ctx.mqtt_client = nullptr;
    }

    s_ctx.status = CLOUD_DISCONNECTED;

    ESP_LOGI(TAG, "Bambu Cloud客户端已停止");
    return ESP_OK;
}

/**
 * @brief 获取当前连接状态
 */
cloud_status_t bambu_cloud_get_status(void) {
    return s_ctx.status;
}

/**
 * @brief 获取最新打印机状态
 */
const printer_state_t* bambu_cloud_get_state(void) {
    return &s_ctx.printer_state;
}

/**
 * @brief 发送命令到打印机
 */
int bambu_cloud_send_command(const char *command, const char *param) {
    ESP_LOGI(TAG, "发送命令: %s (参数: %s)", command, param ? param : "无");
    return ESP_OK;
}
