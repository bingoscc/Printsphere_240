/**
 * @file ui.cpp
 * @brief UI主实现
 *
 * 协调LVGL显示和TFT驱动:
 * - 初始化TFT屏幕和LVGL
 * - 管理屏幕切换
 * - 处理状态更新
 *
 * 显示流程:
 * 1. LVGL绘制到内存缓冲区
 * 2. flush回调将数据发送到TFT
 */

#include "ui.h"
#include "screens.h"
#include "styles.h"
#include "tft_drivers.h"
#include "board_pins.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/spi_master.h"

// 日志TAG
static const char *TAG = "UI";

// 当前屏幕
static ui_screen_t s_current_screen = SCREEN_BOOT;
// 是否已初始化
static bool s_initialized = false;
// 打印机状态
static printer_state_t s_printer_state = {};
// WiFi IP
static char s_wifi_ip[16] = "0.0.0.0";
// WiFi连接状态
static bool s_wifi_connected = false;
// 云端状态
static char s_cloud_status[32] = "Disconnected";

// TFT上下文 (使用genshin_godeye引脚)
static tft_context_t s_tft_ctx = {
    .cs = GPIO_NUM_11,
    .dc = GPIO_NUM_10,
    .rst = GPIO_NUM_21,
    .blk = GPIO_NUM_14,
    .mosi = GPIO_NUM_13,
    .miso = GPIO_NUM_NC,
    .sck = GPIO_NUM_12,
    .width = 240,
    .height = 240,
    .spi_host = SPI2_HOST,
    .spi_freq = 40000000,
    .driver = TFT_DRIVER_GC9A01,
    .spi = nullptr
};

/**
 * @brief LVGL刷新回调
 *
 * 将LVGL渲染的像素数据通过SPI发送到TFT
 */
static void ui_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint16_t *colors = (uint16_t *)px_map;
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    tft_set_window(&s_tft_ctx, area->x1, area->y1, w, h);

    // 分配DMA缓冲区
    uint8_t *buf = (uint8_t *)heap_caps_malloc(w * h * 2, MALLOC_CAP_DMA);
    if (buf) {
        // 转换颜色格式 (LVGL -> RGB565)
        for (int i = 0; i < w * h; i++) {
            buf[i * 2] = (colors[i] >> 8) & 0xFF;
            buf[i * 2 + 1] = colors[i] & 0xFF;
        }
        spi_transaction_t t = {
            .flags = 0,
            .length = w * h * 16,
            .tx_buffer = buf
        };
        spi_device_transmit(s_tft_ctx.spi, &t);
        free(buf);
    }

    lv_display_flush_ready(disp);
}

/**
 * @brief LVGL像素设置回调
 */
static void ui_setpx_cb(lv_display_t *disp, uint8_t *buf, lv_coord_t buf_w,
                         lv_coord_t x, lv_coord_t y, lv_color_t c, lv_color_t full) {
    uint16_t pix = lv_color_to_u16(c);
    buf += 2 * (y * buf_w + x);
    buf[0] = (pix >> 8) & 0xFF;
    buf[1] = pix & 0xFF;
}

// LVGL显示缓冲区
static uint8_t *ui_buf;

/**
 * @brief 初始化UI系统
 */
int ui_init(void) {
    ESP_LOGI(TAG, "正在初始化UI");

    // 初始化TFT
    int ret = tft_init(&s_tft_ctx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TFT初始化失败");
        return ret;
    }

    // 初始化LVGL
    lv_init();

    // 分配显示缓冲区 (240x240x2 = 115200字节)
    ui_buf = (uint8_t *)heap_caps_malloc(240 * 240 * 2, MALLOC_CAP_32BIT);
    if (!ui_buf) {
        ESP_LOGE(TAG, "LVGL缓冲区分配失败");
        return ESP_FAIL;
    }

    // 创建显示
    lv_display_t *disp = lv_display_create(240, 240);
    lv_display_set_buffers(disp, ui_buf, NULL, 240 * 240 * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, ui_flush_cb);
    lv_display_set_set_px_cb(disp, ui_setpx_cb);

    // 初始化样式和屏幕
    styles_init();
    screens_init();

    s_initialized = true;
    ESP_LOGI(TAG, "UI初始化完成");
    return ESP_OK;
}

/**
 * @brief 启动UI系统
 */
int ui_start(void) {
    if (!s_initialized) {
        ESP_LOGE(TAG, "UI未初始化");
        return ESP_FAIL;
    }

    // 显示初始屏幕
    screens_show_status();
    screens_update_wifi(false, NULL);
    screens_update_cloud("初始化中");

    ESP_LOGI(TAG, "UI已启动");
    return ESP_OK;
}

/**
 * @brief 停止UI系统
 */
int ui_stop(void) {
    return ESP_OK;
}

/**
 * @brief 切换屏幕
 */
void ui_set_screen(ui_screen_t screen) {
    s_current_screen = screen;

    switch (screen) {
        case SCREEN_STATUS:
            screens_show_status();
            break;
        case SCREEN_SETTINGS:
            screens_show_settings();
            break;
        default:
            break;
    }
}

/**
 * @brief 更新打印机状态显示
 */
void ui_update_state(const printer_state_t *state) {
    if (state) {
        memcpy(&s_printer_state, state, sizeof(printer_state_t));
        screens_update_status(state);
    }
}

/**
 * @brief 更新WiFi状态
 */
void ui_set_wifi_state(bool connected, const char *ip) {
    s_wifi_connected = connected;
    if (ip) {
        strncpy(s_wifi_ip, ip, sizeof(s_wifi_ip) - 1);
    }
    screens_update_wifi(connected, ip);
}

/**
 * @brief 更新云端状态
 */
void ui_set_cloud_state(const char *status) {
    if (status) {
        strncpy(s_cloud_status, status, sizeof(s_cloud_status) - 1);
    }
    screens_update_cloud(status);
}

/**
 * @brief 刷新UI
 */
void ui_invalidate(void) {
    if (s_initialized) {
        lv_obj_invalidate(lv_scr_act());
    }
}
