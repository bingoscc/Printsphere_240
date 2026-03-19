/**
 * @file config_store.cpp
 * @brief 配置存储实现
 *
 * 使用NVS (Non-Volatile Storage) 持久化存储配置数据
 * 支持掉电保存，下次启动自动加载
 */

#include "config_store.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

// 日志TAG
static const char *TAG = "ConfigStore";
// NVS命名空间
static const char *NVS_NAMESPACE = "printsphere";
// 配置键名
static const char *KEY_CONFIG = "config";

/**
 * @brief 初始化NVS存储
 *
 * 如果NVS分区损坏或版本不兼容，会自动擦除重建
 */
int config_store_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS分区已满或版本不匹配，擦除后重新初始化
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "配置存储初始化完成");
    return ESP_OK;
}

/**
 * @brief 默认配置
 *
 * 当NVS中没有保存的配置时使用此默认值
 */
static printsphere_config_t default_config = {
    .wifi_ssid = "",
    .wifi_password = "",
    .bambu_username = "",
    .bambu_password = "",
    .bambu_device_serial = "",
    .mqtt_host = "192.168.1.100",  // 默认HASS地址
    .mqtt_port = 1883,              // 默认MQTT端口
    .mqtt_username = "",
    .mqtt_password = "",
    .display_rotation = 0,
    .hass_enabled = true,           // 默认启用HASS
    .cloud_enabled = true,          // 默认启用Bambu Cloud
    .button_long_press_ms = 600,    // 长按阈值600ms
    .button_short_press_ms = 300     // 短按阈值300ms
};

/**
 * @brief 从NVS获取配置
 *
 * @param cfg 输出参数，存储获取的配置
 * @return ESP_OK成功，ESP_FAIL使用默认值
 */
int config_store_get(printsphere_config_t *cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        // NVS打开失败，使用默认值
        ESP_LOGW(TAG, "NVS打开失败，使用默认值: %s", esp_err_to_name(err));
        *cfg = default_config;
        return ESP_FAIL;
    }

    size_t required_size = sizeof(printsphere_config_t);
    err = nvs_get_blob(handle, KEY_CONFIG, cfg, &required_size);
    if (err != ESP_OK) {
        // 配置读取失败，使用默认值
        ESP_LOGW(TAG, "NVS读取配置失败，使用默认值: %s", esp_err_to_name(err));
        *cfg = default_config;
    }

    nvs_close(handle);
    return ESP_OK;
}

/**
 * @brief 保存配置到NVS
 *
 * @param cfg 输入参数，要保存的配置
 * @return ESP_OK成功
 */
int config_store_set(const printsphere_config_t *cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS打开失败: %s", esp_err_to_name(err));
        return err;
    }

    // 将配置结构体写入NVS
    err = nvs_set_blob(handle, KEY_CONFIG, cfg, sizeof(printsphere_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS写入配置失败: %s", esp_err_to_name(err));
    } else {
        // 提交更改
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "NVS提交失败: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "配置已保存到NVS");
        }
    }

    nvs_close(handle);
    return err;
}

/**
 * @brief 重置配置为默认值
 *
 * 删除NVS中保存的配置，恢复出厂设置
 */
int config_store_reset(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    // 删除配置键
    err = nvs_erase_key(handle, KEY_CONFIG);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS删除配置失败: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "配置已重置为默认值");
    }

    nvs_close(handle);
    return err;
}

/**
 * @brief 检查设备是否已完成配网
 *
 * 通过检查WiFi SSID是否配置过来判断
 * @return true 已配网，false 未配网
 */
bool config_store_is_provisioned(void) {
    printsphere_config_t cfg;
    config_store_get(&cfg);
    return strlen(cfg.wifi_ssid) > 0;
}
