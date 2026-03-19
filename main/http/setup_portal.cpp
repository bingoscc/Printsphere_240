/**
 * @file setup_portal.cpp
 * @brief Web配网页面实现
 *
 * 首次启动时提供Web配网界面，用户通过浏览器配置:
 * - WiFi SSID和密码
 * - Bambu Lab账号
 * - HomeAssistant MQTT设置
 *
 * HTTP服务器运行在80端口，提供简洁的HTML界面
 */

#include "setup_portal.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <string.h>

// 日志TAG
static const char *TAG = "SetupPortal";
// 默认AP名称
static const char *SETUP_AP_SSID = "PrintSphere-Setup";
static const char *SETUP_AP_PASSWORD = "";

// HTTP服务器句柄
static httpd_handle_t s_httpd_handle = nullptr;
// 配网完成回调
static setup_complete_callback_t s_complete_callback = nullptr;
static void *s_callback_user_data = nullptr;

/**
 * @brief 配网页面HTML
 *
 * 包含:
 * - WiFi SSID/密码输入和连接按钮
 * - Bambu Lab账号输入
 * - HASS MQTT设置
 * - 保存配置按钮
 */
static const char *html_page =
"<!DOCTYPE html><html><head>"
"<meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<title>PrintSphere Setup</title>"
"<style>"
"body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#1a1a2e;color:#fff;}"
".container{max-width:400px;margin:0 auto;}"
"h1{text-align:center;color:#e94560;}"
".card{background:#16213e;padding:20px;border-radius:10px;margin-bottom:20px;}"
"label{display:block;margin-bottom:5px;color:#aaa;}"
"input{width:100%;padding:10px;margin-bottom:15px;border:none;border-radius:5px;"
"background:#0f3460;color:#fff;box-sizing:border-box;}"
"button{width:100%;padding:12px;background:#e94560;border:none;border-radius:5px;"
"color:#fff;font-size:16px;cursor:pointer;}"
"button:hover{background:#ff6b6b;}"
".status{margin-top:20px;padding:10px;border-radius:5px;text-align:center;}"
".status.success{background:#00b894;}"
".status.error{background:#e94560;}"
".hidden{display:none;}"
"</style></head><body>"
"<div class=\"container\">"
"<h1>PrintSphere Setup</h1>"
"<div class=\"card\">"
"<label>WiFi SSID</label>"
"<input type=\"text\" id=\"ssid\" placeholder=\"WiFi名称\">"
"<label>WiFi Password</label>"
"<input type=\"password\" id=\"password\" placeholder=\"WiFi密码\">"
"<button onclick=\"connect()\">连接</button>"
"</div>"
"<div class=\"card\">"
"<label>Bambu Username (Email)</label>"
"<input type=\"text\" id=\"bambu_user\" placeholder=\"Bambu Lab账号邮箱\">"
"<label>Bambu Password</label>"
"<input type=\"password\" id=\"bambu_pass\" placeholder=\"Bambu Lab密码\">"
"<label>Device Serial</label>"
"<input type=\"text\" id=\"serial\" placeholder=\"设备序列号\">"
"</div>"
"<div class=\"card\">"
"<label>HomeAssistant MQTT Host</label>"
"<input type=\"text\" id=\"mqtt_host\" placeholder=\"192.168.1.100\" value=\"192.168.1.100\">"
"<label>MQTT Port</label>"
"<input type=\"number\" id=\"mqtt_port\" placeholder=\"1883\" value=\"1883\">"
"</div>"
"<button onclick=\"save()\">保存配置</button>"
"<div id=\"status\" class=\"status hidden\"></div>"
"</div>"
"<script>"
"function showStatus(msg, isError) {"
"var s=document.getElementById('status');"
"s.textContent=msg;"
"s.className='status '+(isError?'error':'success');"
"s.classList.remove('hidden');"
"}"
"function connect(){"
"var ssid=document.getElementById('ssid').value;"
"var pass=document.getElementById('password').value;"
"if(!ssid){showStatus('请输入SSID',true);return;}"
"fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass))"
".then(r=>r.json()).then(d=>{"
"if(d.success)showStatus('WiFi连接成功！');else showStatus('连接失败: '+d.error,true);"
"}).catch(e=>showStatus('请求失败',true));"
"}"
"function save(){"
"var data={"
"ssid:document.getElementById('ssid').value,"
"password:document.getElementById('password').value,"
"bambu_user:document.getElementById('bambu_user').value,"
"bambu_pass:document.getElementById('bambu_pass').value,"
"serial:document.getElementById('serial').value,"
"mqtt_host:document.getElementById('mqtt_host').value,"
"mqtt_port:parseInt(document.getElementById('mqtt_port').value)"
"};"
"fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},"
"body:JSON.stringify(data)})"
".then(r=>r.json()).then(d=>{"
"if(d.success){showStatus('配置已保存，设备将重启');setTimeout(()=>location.reload(),2000);}"
"else showStatus('保存失败',true);"
"}).catch(e=>showStatus('请求失败',true));"
"}"
"</script></body></html>";

/**
 * @brief 根路径处理器 - 返回配网页面
 */
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_page, strlen(html_page));
    return ESP_OK;
}

/**
 * @brief /connect 处理器 - 发起WiFi连接
 *
 * 参数:
 * - ssid: WiFi名称
 * - pass: WiFi密码
 */
static esp_err_t connect_handler(httpd_req_t *req) {
    char ssid[64] = {};
    char pass[128] = {};
    char buf[256] = {};
    int buf_len = sizeof(buf);

    // 解析URL参数
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
        httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
        httpd_query_key_value(buf, "pass", pass, sizeof(pass));
    }

    ESP_LOGI(TAG, "WiFi连接请求: ssid=%s", ssid);

    // 配置WiFi
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (strlen(pass) > 0) {
        strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = strlen(pass) > 0 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    snprintf(buf, sizeof(buf), "{\"success\":true}");
    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}

/**
 * @brief /save 处理器 - 保存配置
 *
 * 接收JSON格式的配置数据并保存到NVS
 */
static esp_err_t save_handler(httpd_req_t *req) {
    char buf[1024] = {};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    ESP_LOGI(TAG, "保存配置: %s", buf);

    // TODO: 解析JSON并保存到NVS
    // 目前仅返回成功响应

    snprintf(buf, sizeof(buf), "{\"success\":true}");
    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}

/**
 * @brief /status 处理器 - 获取WiFi状态
 */
static esp_err_t status_handler(httpd_req_t *req) {
    char buf[256] = {};
    wifi_ap_record_t ap_info = {};
    bool connected = (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK);

    snprintf(buf, sizeof(buf), "{\"connected\":%s,\"ssid\":\"%s\",\"rssi\":%d}",
             connected ? "true" : "false",
             connected ? (char *)ap_info.ssid : "",
             connected ? ap_info.rssi : 0);

    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}

// HTTP URI路由表
static httpd_uri_t sURIs[] = {
    {.uri = "/", .method = HTTP_GET, .handler = root_handler},          // 配网页面
    {.uri = "/connect", .method = HTTP_GET, .handler = connect_handler}, // WiFi连接
    {.uri = "/save", .method = HTTP_POST, .handler = save_handler},     // 保存配置
    {.uri = "/status", .method = HTTP_GET, .handler = status_handler},  // WiFi状态
};

/**
 * @brief 初始化配网Portal
 */
int setup_portal_init(const char *ssid) {
    if (ssid) {
        SETUP_AP_SSID = ssid;
    }
    return ESP_OK;
}

/**
 * @brief 启动HTTP服务器
 */
int setup_portal_start(void) {
    if (s_httpd_handle) {
        ESP_LOGW(TAG, "HTTP服务器已在运行");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 4096;
    config.server_port = 80;

    if (httpd_start(&s_httpd_handle, &config) == ESP_OK) {
        // 注册URI处理器
        for (int i = 0; i < sizeof(sURIs) / sizeof(sURIs[0]); i++) {
            httpd_register_uri_handler(s_httpd_handle, &sURIs[i]);
        }
        ESP_LOGI(TAG, "配网Portal HTTP服务器已启动，端口80");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "HTTP服务器启动失败");
    return ESP_FAIL;
}

/**
 * @brief 停止HTTP服务器
 */
int setup_portal_stop(void) {
    if (s_httpd_handle) {
        httpd_stop(s_httpd_handle);
        s_httpd_handle = nullptr;
        ESP_LOGI(TAG, "配网Portal HTTP服务器已停止");
    }
    return ESP_OK;
}

/**
 * @brief 设置完成回调
 */
int setup_portal_set_complete_callback(setup_complete_callback_t callback, void *user_data) {
    s_complete_callback = callback;
    s_callback_user_data = user_data;
    return ESP_OK;
}

/**
 * @brief 检查Portal是否运行中
 */
bool setup_portal_is_running(void) {
    return s_httpd_handle != nullptr;
}
