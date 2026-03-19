/**
 * @file config_store.h
 * @brief 配置存储接口
 *
 * 使用ESP32 NVS (Non-Volatile Storage) 存储配置数据
 * 配置数据包括:
 * - WiFi SSID和密码
 * - Bambu Cloud用户名密码
 * - HomeAssistant MQTT设置
 * - 显示和按钮配置
 */

#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PrintSphere 配置结构体
 */
typedef struct {
    char wifi_ssid[64];           // WiFi名称
    char wifi_password[128];      // WiFi密码
    char bambu_username[128];     // Bambu Lab用户名(邮箱)
    char bambu_password[128];     // Bambu Lab密码
    char bambu_device_serial[32];  // 设备序列号
    char mqtt_host[128];          // HomeAssistant MQTT地址
    int mqtt_port;               // MQTT端口 (默认1883)
    char mqtt_username[64];       // MQTT用户名
    char mqtt_password[128];     // MQTT密码
    int display_rotation;        // 屏幕旋转角度
    bool hass_enabled;           // 是否启用HASS集成
    bool cloud_enabled;          // 是否启用Bambu Cloud
    int button_long_press_ms;     // 按键长按阈值(毫秒)
    int button_short_press_ms;    // 按键短按阈值(毫秒)
} printsphere_config_t;

/**
 * @brief 初始化配置存储
 * @return ESP_OK 成功
 */
int config_store_init(void);

/**
 * @brief 获取当前配置
 * @param cfg 输出参数，配置结构体指针
 * @return ESP_OK 成功，ESP_FAIL 使用默认值
 */
int config_store_get(printsphere_config_t *cfg);

/**
 * @brief 保存配置到NVS
 * @param cfg 输入参数，要保存的配置
 * @return ESP_OK 成功
 */
int config_store_set(const printsphere_config_t *cfg);

/**
 * @brief 重置配置为默认值
 * @return ESP_OK 成功
 */
int config_store_reset(void);

/**
 * @brief 检查设备是否已完成配网
 * @return true 已配网，false 未配网
 */
bool config_store_is_provisioned(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_STORE_H
