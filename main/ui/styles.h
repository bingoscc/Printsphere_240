/**
 * @file styles.h
 * @brief LVGL样式定义
 *
 * 统一管理UI颜色和尺寸参数
 *
 * 颜色定义 (暗色主题):
 * - 背景: #1a1a2e (深蓝黑)
 * - 卡片: #16213e (深蓝)
 * - 强调色: #e94560 (红色)
 * - 文字: #ffffff (白色)
 * - 文字暗: #888888 (灰色)
 * - 成功: #00b894 (绿色)
 * - 警告: #fdcb6e (黄色)
 * - 错误: #e94560 (红色)
 */

#ifndef STYLES_H
#define STYLES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 颜色定义
#define COLOR_BG_DARK    lv_color_hex(0x1a1a2e)   // 深色背景
#define COLOR_BG_CARD    lv_color_hex(0x16213e)   // 卡片背景
#define COLOR_ACCENT     lv_color_hex(0xe94560)   // 强调色
#define COLOR_TEXT       lv_color_hex(0xffffff)   // 主文字
#define COLOR_TEXT_DIM   lv_color_hex(0x888888)  // 次要文字
#define COLOR_SUCCESS    lv_color_hex(0x00b894)  // 成功状态
#define COLOR_WARNING    lv_color_hex(0xfdcb6e)  // 警告状态
#define COLOR_ERROR      lv_color_hex(0xe94560)  // 错误状态

// 显示屏尺寸
#define DISP_WIDTH   240   // 屏幕宽度
#define DISP_HEIGHT  240   // 屏幕高度

// 进度环尺寸 (适配240x240屏幕)
#define RING_CENTER_X   120   // 圆心X
#define RING_CENTER_Y   105   // 圆心Y
#define RING_RADIUS     75    // 外半径
#define RING_THICKNESS  12    // 环宽度

/**
 * @brief 初始化所有样式
 */
void styles_init(void);

/**
 * @brief 获取卡片样式
 */
lv_style_t* style_get_card(void);

/**
 * @brief 获取标题样式
 */
lv_style_t* style_get_title(void);

/**
 * @brief 获取正文样式
 */
lv_style_t* style_get_body(void);

/**
 * @brief 获取标签样式
 */
lv_style_t* style_get_label(void);

/**
 * @brief 获取按钮样式
 */
lv_style_t* style_get_button(void);

#ifdef __cplusplus
}
#endif

#endif // STYLES_H
