/**
 * @file screens.cpp
 * @brief LVGL屏幕实现
 *
 * 240x240显示屏布局:
 * - 顶部(0-30): Logo区域
 * - 中部(30-180): 圆形进度环 + 百分比
 * - 底部(180-240): 状态信息
 *
 * 包含两个主要屏幕:
 * - 状态屏幕: 实时显示打印进度
 * - 设置屏幕: WiFi/云端状态、重启/重置按钮
 */

#include "screens.h"
#include "styles.h"
#include "progress_ring.h"
#include <string.h>

// 全局屏幕实例
status_screen_t g_status_screen = {};
settings_screen_t g_settings_screen = {};

// 进度环实例
static progress_ring_t s_progress_ring;
static printer_state_t s_last_state = {};

/**
 * @brief 初始化进度环
 *
 * 创建Canvas并设置进度环参数
 */
static void draw_progress_ring(lv_obj_t *parent, progress_ring_t *ring) {
    // 创建Canvas
    lv_obj_t *canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, heap_caps_malloc(240 * 240 * 2, MALLOC_CAP_32BIT), 240, 240, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(canvas, 0, 0);

    // 设置进度环参数
    ring->x = RING_CENTER_X;
    ring->y = RING_CENTER_Y;
    ring->radius = RING_RADIUS;
    ring->thickness = RING_THICKNESS;
    ring->bg_color = lv_color_hex(0x333355);
    ring->fg_color = lv_color_hex(0xe94560);
    ring->progress = 0;
}

/**
 * @brief 初始化所有屏幕
 */
int screens_init(void) {
    // ========== 状态屏幕 ==========
    g_status_screen.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_status_screen.screen, COLOR_BG_DARK, 0);

    // Logo标签
    g_status_screen.logo = lv_label_create(g_status_screen.screen);
    lv_label_set_text(g_status_screen.logo, "Bambu\nPrintSphere");
    lv_obj_set_style_text_align(g_status_screen.logo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_status_screen.logo, 0, 5);
    lv_obj_set_width(g_status_screen.logo, 240);

    // 进度环
    draw_progress_ring(g_status_screen.screen, &s_progress_ring);

    // 进度百分比标签
    g_status_screen.progress_label = lv_label_create(g_status_screen.screen);
    lv_label_set_text(g_status_screen.progress_label, "0%");
    lv_obj_set_style_text_align(g_status_screen.progress_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_status_screen.progress_label, 0, 90);
    lv_obj_set_width(g_status_screen.progress_label, 240);
    lv_obj_set_style_text_color(g_status_screen.progress_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_status_screen.progress_label, &lv_font_montserrat_24, 0);

    // 状态标签
    g_status_screen.status_label = lv_label_create(g_status_screen.screen);
    lv_label_set_text(g_status_screen.status_label, "连接中...");
    lv_obj_set_style_text_align(g_status_screen.status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_status_screen.status_label, 0, 185);
    lv_obj_set_width(g_status_screen.status_label, 240);
    lv_obj_set_style_text_color(g_status_screen.status_label, lv_color_hex(0x00b894), 0);
    lv_obj_set_style_text_font(g_status_screen.status_label, &lv_font_montserrat_14, 0);

    // 温度标签
    g_status_screen.temp_label = lv_label_create(g_status_screen.screen);
    lv_label_set_text(g_status_screen.temp_label, "喷嘴: --°C  床: --°C");
    lv_obj_set_style_text_align(g_status_screen.temp_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_status_screen.temp_label, 0, 205);
    lv_obj_set_width(g_status_screen.temp_label, 240);
    lv_obj_set_style_text_color(g_status_screen.temp_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(g_status_screen.temp_label, &lv_font_montserrat_12, 0);

    // 剩余时间标签
    g_status_screen.time_label = lv_label_create(g_status_screen.screen);
    lv_label_set_text(g_status_screen.time_label, "剩余: --:--");
    lv_obj_set_style_text_align(g_status_screen.time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_status_screen.time_label, 0, 185);
    lv_obj_set_width(g_status_screen.time_label, 240);
    lv_obj_set_style_text_color(g_status_screen.time_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(g_status_screen.time_label, &lv_font_montserrat_12, 0);

    // 作业名称标签
    g_status_screen.job_label = lv_label_create(g_status_screen.screen);
    lv_label_set_text(g_status_screen.job_label, "");
    lv_obj_set_style_text_align(g_status_screen.job_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_status_screen.job_label, 0, 220);
    lv_obj_set_width(g_status_screen.job_label, 240);
    lv_obj_set_style_text_color(g_status_screen.job_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(g_status_screen.job_label, &lv_font_montserrat_10, 0);

    // ========== 设置屏幕 ==========
    g_settings_screen.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_settings_screen.screen, COLOR_BG_DARK, 0);

    // 标题
    g_settings_screen.title = lv_label_create(g_settings_screen.screen);
    lv_label_set_text(g_settings_screen.title, "设置");
    lv_obj_set_style_text_align(g_settings_screen.title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(g_settings_screen.title, 0, 10);
    lv_obj_set_width(g_settings_screen.title, 240);
    lv_obj_set_style_text_color(g_settings_screen.title, lv_color_hex(0xe94560), 0);
    lv_obj_set_style_text_font(g_settings_screen.title, &lv_font_montserrat_18, 0);

    // IP地址标签
    g_settings_screen.ip_label = lv_label_create(g_settings_screen.screen);
    lv_label_set_text(g_settings_screen.ip_label, "IP: --");
    lv_obj_set_pos(g_settings_screen.ip_label, 20, 50);
    lv_obj_set_style_text_color(g_settings_screen.ip_label, lv_color_hex(0xffffff), 0);

    // WiFi状态标签
    g_settings_screen.ssid_label = lv_label_create(g_settings_screen.screen);
    lv_label_set_text(g_settings_screen.ssid_label, "WiFi: --");
    lv_obj_set_pos(g_settings_screen.ssid_label, 20, 75);
    lv_obj_set_style_text_color(g_settings_screen.ssid_label, lv_color_hex(0x888888), 0);

    // 云端状态标签
    g_settings_screen.cloud_status_label = lv_label_create(g_settings_screen.screen);
    lv_label_set_text(g_settings_screen.cloud_status_label, "云端: --");
    lv_obj_set_pos(g_settings_screen.cloud_status_label, 20, 100);
    lv_obj_set_style_text_color(g_settings_screen.cloud_status_label, lv_color_hex(0x888888), 0);

    // 重置配置按钮
    g_settings_screen.btn_reset = lv_btn_create(g_settings_screen.screen);
    lv_obj_set_pos(g_settings_screen.btn_reset, 20, 150);
    lv_obj_set_width(g_settings_screen.btn_reset, 200);
    lv_obj_t *reset_label = lv_label_create(g_settings_screen.btn_reset);
    lv_label_set_text(reset_label, "重置配置");
    lv_obj_set_style_text_color(reset_label, lv_color_hex(0xffffff), 0);

    // 重启按钮
    g_settings_screen.btn_reboot = lv_btn_create(g_settings_screen.screen);
    lv_obj_set_pos(g_settings_screen.btn_reboot, 20, 195);
    lv_obj_set_width(g_settings_screen.btn_reboot, 200);
    lv_obj_t *reboot_label = lv_label_create(g_settings_screen.btn_reboot);
    lv_label_set_text(reboot_label, "重启设备");
    lv_obj_set_style_text_color(reboot_label, lv_color_hex(0xffffff), 0);

    printer_state_init(&s_last_state);
    progress_ring_init(&s_progress_ring);

    return 0;
}

/**
 * @brief 显示状态屏幕
 */
void screens_show_status(void) {
    lv_scr_load(g_status_screen.screen);
}

/**
 * @brief 显示设置屏幕
 */
void screens_show_settings(void) {
    lv_scr_load(g_settings_screen.screen);
}

/**
 * @brief 更新状态屏幕
 *
 * 根据打印机状态更新显示内容
 */
void screens_update_status(const printer_state_t *state) {
    if (!state) return;

    // 更新进度
    char progress_text[32];
    snprintf(progress_text, sizeof(progress_text), "%.0f%%", state->progress);
    lv_label_set_text(g_status_screen.progress_label, progress_text);
    s_progress_ring.progress = state->progress;

    // 更新状态文字
    const char *status_str = printer_status_to_string(state->status);
    lv_label_set_text(g_status_screen.status_label, status_str);

    // 更新温度
    char temp_text[64];
    snprintf(temp_text, sizeof(temp_text), "喷嘴: %.0f/%.0f°C  床: %.0f/%.0f°C",
             state->nozzle_temp, state->nozzle_target_temp,
             state->bed_temp, state->bed_target_temp);
    lv_label_set_text(g_status_screen.temp_label, temp_text);

    // 更新剩余时间
    char time_text[32];
    if (state->remaining_time > 0) {
        int hours = state->remaining_time / 3600;
        int minutes = (state->remaining_time % 3600) / 60;
        if (hours > 0) {
            snprintf(time_text, sizeof(time_text), "剩余: %dh %dm", hours, minutes);
        } else {
            snprintf(time_text, sizeof(time_text), "剩余: %dm", minutes);
        }
    } else {
        snprintf(time_text, sizeof(time_text), "剩余: --");
    }
    lv_label_set_text(g_status_screen.time_label, time_text);

    // 更新作业名称
    if (strlen(state->job_name) > 0) {
        lv_label_set_text(g_status_screen.job_label, state->job_name);
    }

    // 根据状态更新颜色
    if (state->status == PRINTER_PRINTING) {
        lv_obj_set_style_text_color(g_status_screen.status_label, lv_color_hex(0x00b894), 0);
    } else if (state->status == PRINTER_ERROR) {
        lv_obj_set_style_text_color(g_status_screen.status_label, lv_color_hex(0xe94560), 0);
    } else if (state->status == PRINTER_PAUSED) {
        lv_obj_set_style_text_color(g_status_screen.status_label, lv_color_hex(0xfdcb6e), 0);
    }

    memcpy(&s_last_state, state, sizeof(printer_state_t));
}

/**
 * @brief 更新WiFi状态显示
 */
void screens_update_wifi(bool connected, const char *ip) {
    char buf[64];
    if (connected) {
        snprintf(buf, sizeof(buf), "IP: %s", ip ? ip : "unknown");
        lv_label_set_text(g_settings_screen.ip_label, buf);
        lv_label_set_text_fmt(g_settings_screen.ssid_label, "WiFi: 已连接");
        lv_obj_set_style_text_color(g_settings_screen.ssid_label, lv_color_hex(0x00b894), 0);
    } else {
        lv_label_set_text(g_settings_screen.ip_label, "IP: --");
        lv_label_set_text(g_settings_screen.ssid_label, "WiFi: 未连接");
        lv_obj_set_style_text_color(g_settings_screen.ssid_label, lv_color_hex(0xe94560), 0);
    }
}

/**
 * @brief 更新云端状态显示
 */
void screens_update_cloud(const char *status) {
    char buf[64];
    snprintf(buf, sizeof(buf), "云端: %s", status ? status : "未知");
    lv_label_set_text(g_settings_screen.cloud_status_label, buf);
}
