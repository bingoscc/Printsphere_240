/**
 * @file styles.cpp
 * @brief LVGL样式实现
 *
 * 统一管理暗色主题的样式定义
 */

#include "styles.h"
#include "lvgl.h"

// 样式实例
static lv_style_t s_style_card;
static lv_style_t s_style_title;
static lv_style_t s_style_body;
static lv_style_t s_style_label;
static lv_style_t s_style_button;

/**
 * @brief 初始化所有样式
 *
 * 创建暗色主题的UI组件样式:
 * - 卡片样式: 深蓝色背景，圆角
 * - 标题样式: 红色强调色
 * - 正文样式: 白色文字
 * - 标签样式: 灰色文字
 * - 按钮样式: 红色背景
 */
void styles_init(void) {
    // 卡片样式
    lv_style_init(&s_style_card);
    lv_style_set_bg_color(&s_style_card, COLOR_BG_CARD);
    lv_style_set_radius(&s_style_card, 10);   // 圆角10px
    lv_style_set_pad_all(&s_style_card, 10); // 内边距10px

    // 标题样式
    lv_style_init(&s_style_title);
    lv_style_set_text_color(&s_style_title, COLOR_ACCENT);
    lv_style_set_text_font(&s_style_title, &lv_font_montserrat_16);

    // 正文样式
    lv_style_init(&s_style_body);
    lv_style_set_text_color(&s_style_body, COLOR_TEXT);
    lv_style_set_text_font(&s_style_body, &lv_font_montserrat_14);

    // 标签样式
    lv_style_init(&s_style_label);
    lv_style_set_text_color(&s_style_label, COLOR_TEXT_DIM);
    lv_style_set_text_font(&s_style_label, &lv_font_montserrat_12);

    // 按钮样式
    lv_style_init(&s_style_button);
    lv_style_set_bg_color(&s_style_button, COLOR_ACCENT);
    lv_style_set_radius(&s_style_button, 5);
    lv_style_set_text_color(&s_style_button, COLOR_TEXT);
}

/**
 * @brief 获取卡片样式
 */
lv_style_t* style_get_card(void) { return &s_style_card; }
/**
 * @brief 获取标题样式
 */
lv_style_t* style_get_title(void) { return &s_style_title; }
/**
 * @brief 获取正文样式
 */
lv_style_t* style_get_body(void) { return &s_style_body; }
/**
 * @brief 获取标签样式
 */
lv_style_t* style_get_label(void) { return &s_style_label; }
/**
 * @brief 获取按钮样式
 */
lv_style_t* style_get_button(void) { return &s_style_button; }
