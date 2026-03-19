/**
 * @file printer_client.cpp
 * @brief 本地打印机通信实现
 *
 * 通过HTTP API轮询本地打印机获取状态
 * 轮询间隔: 3秒
 *
 * API端点:
 * - GET /api/printer/status - 获取打印机状态
 * - POST /api/printer/command - 发送命令
 */

#include "printer_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>

// 日志TAG
static const char *TAG = "PrinterClient";

/**
 * @brief 打印机客户端上下文
 */
typedef struct {
    char host[128];                       // 打印机IP地址
    int port;                            // HTTP端口
    bool initialized;                    // 是否已初始化
    bool running;                        // 是否正在运行
    printer_state_t printer_state;       // 打印机状态
    printer_client_callback_t callback;   // 状态回调
    void *user_data;                     // 用户数据
    esp_timer_handle_t poll_timer;       // 轮询定时器
    char last_known_ip[16];              // 上次已知IP
} printer_client_context_t;

static printer_client_context_t s_ctx = {};

static void poll_timer_callback(void *arg);

/**
 * @brief 发送HTTP GET请求到打印机
 *
 * @param path API路径
 * @param response 响应缓冲区
 * @param resp_len 缓冲区长度
 */
static int printer_http_get(const char *path, char *response, size_t resp_len) {
    char url[384];
    snprintf(url, sizeof(url), "http://%s:%d%s", s_ctx.host, s_ctx.port, path);

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    int err = esp_http_client_open(client, 0);
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
 * @brief 解析打印机状态JSON
 *
 * 从HTTP响应中提取关键状态信息
 */
static int printer_parse_status(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return ESP_FAIL;
    }

    // 解析打印机状态
    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (state) {
        if (strcmp(state->valuestring, "printing") == 0) {
            s_ctx.printer_state.status = PRINTER_PRINTING;
        } else if (strcmp(state->valuestring, "idle") == 0) {
            s_ctx.printer_state.status = PRINTER_IDLE;
        } else if (strcmp(state->valuestring, "paused") == 0) {
            s_ctx.printer_state.status = PRINTER_PAUSED;
        } else if (strcmp(state->valuestring, "complete") == 0) {
            s_ctx.printer_state.status = PRINTER_COMPLETED;
        } else if (strcmp(state->valuestring, "error") == 0) {
            s_ctx.printer_state.status = PRINTER_ERROR;
        }
    }

    // 解析打印进度
    cJSON *progress = cJSON_GetObjectItem(root, "progress");
    if (progress) {
        s_ctx.printer_state.progress = (float)progress->valuedouble;
    }

    // 解析作业名称
    cJSON *job_name = cJSON_GetObjectItem(root, "job_name");
    if (job_name) {
        strncpy(s_ctx.printer_state.job_name, job_name->valuestring,
                sizeof(s_ctx.printer_state.job_name) - 1);
    }

    // 解析喷嘴温度
    cJSON *tool0 = cJSON_GetObjectItem(root, "tool0");
    if (tool0) {
        cJSON *temp = cJSON_GetObjectItem(tool0, "temp");
        cJSON *target = cJSON_GetObjectItem(tool0, "target");
        if (temp) s_ctx.printer_state.nozzle_temp = (float)temp->valuedouble;
        if (target) s_ctx.printer_state.nozzle_target_temp = (float)target->valuedouble;
    }

    // 解析床基温度
    cJSON *bed = cJSON_GetObjectItem(root, "bed");
    if (bed) {
        cJSON *temp = cJSON_GetObjectItem(bed, "temp");
        cJSON *target = cJSON_GetObjectItem(bed, "target");
        if (temp) s_ctx.printer_state.bed_temp = (float)temp->valuedouble;
        if (target) s_ctx.printer_state.bed_target_temp = (float)target->valuedouble;
    }

    // 解析剩余时间
    cJSON *remaining = cJSON_GetObjectItem(root, "remaining_time");
    if (remaining) {
        s_ctx.printer_state.remaining_time = remaining->valueint;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief 轮询定时器回调
 *
 * 定时调用HTTP API获取打印机状态
 */
static void poll_timer_callback(void *arg) {
    if (!s_ctx.running) return;

    char response[2048] = {};
    if (printer_http_get("/api/printer/status", response, sizeof(response)) == ESP_OK) {
        printer_parse_status(response);

        if (s_ctx.callback) {
            s_ctx.callback(&s_ctx.printer_state, s_ctx.user_data);
        }
    } else {
        // HTTP请求失败，设置离线状态
        s_ctx.printer_state.status = PRINTER_OFFLINE;
    }
}

/**
 * @brief 初始化打印机客户端
 */
int printer_client_init(const char *host, int port) {
    memset(&s_ctx, 0, sizeof(s_ctx));

    if (host) strncpy(s_ctx.host, host, sizeof(s_ctx.host) - 1);
    s_ctx.port = port > 0 ? port : 7125;  // 默认端口7125

    s_ctx.initialized = true;
    printer_state_init(&s_ctx.printer_state);

    // 创建轮询定时器 (3秒间隔)
    esp_timer_create_args_t timer_args = {
        .callback = poll_timer_callback,
        .arg = &s_ctx,
        .name = "printer_poll"
    };
    esp_timer_create(&timer_args, &s_ctx.poll_timer);

    ESP_LOGI(TAG, "打印机客户端初始化: %s:%d", s_ctx.host, s_ctx.port);
    return ESP_OK;
}

/**
 * @brief 设置状态回调函数
 */
int printer_client_set_callback(printer_client_callback_t callback, void *user_data) {
    s_ctx.callback = callback;
    s_ctx.user_data = user_data;
    return ESP_OK;
}

/**
 * @brief 启动轮询
 */
int printer_client_start(void) {
    if (!s_ctx.initialized) {
        ESP_LOGE(TAG, "打印机客户端未初始化");
        return ESP_FAIL;
    }

    if (s_ctx.running) {
        return ESP_OK;
    }

    s_ctx.running = true;
    esp_timer_start_periodic(s_ctx.poll_timer, 3000000);  // 3秒

    ESP_LOGI(TAG, "打印机客户端已启动");
    return ESP_OK;
}

/**
 * @brief 停止轮询
 */
int printer_client_stop(void) {
    if (!s_ctx.running) {
        return ESP_OK;
    }

    s_ctx.running = false;
    esp_timer_stop(s_ctx.poll_timer);

    ESP_LOGI(TAG, "打印机客户端已停止");
    return ESP_OK;
}

/**
 * @brief 检查是否已连接
 */
bool printer_client_is_connected(void) {
    return s_ctx.running && s_ctx.printer_state.status != PRINTER_OFFLINE;
}

/**
 * @brief 获取最新状态
 */
const printer_state_t* printer_client_get_state(void) {
    return &s_ctx.printer_state;
}

/**
 * @brief 发送命令到打印机
 *
 * @param cmd JSON格式命令字符串
 */
int printer_client_send_command(const char *cmd) {
    char url[384];
    snprintf(url, sizeof(url), "http://%s:%d/api/printer/command", s_ctx.host, s_ctx.port);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_fields(client, cmd, strlen(cmd));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "命令发送失败: %s (status=%d)", esp_err_to_name(err), status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "命令已发送: %s", cmd);
    return ESP_OK;
}
