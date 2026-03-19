/**
 * @file progress_ring.h
 * @brief 圆形进度环控件
 *
 * 用于240x240屏幕显示打印进度
 * 尺寸参数:
 * - 中心点: (120, 105)
 * - 外半径: 75px
 * - 环宽度: 12px
 */

#ifndef PROGRESS_RING_H
#define PROGRESS_RING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 进度环配置结构体
 */
typedef struct {
    int x;              // 圆心X坐标
    int y;              // 圆心Y坐标
    int radius;         // 外半径
    int thickness;      // 环宽度
    uint16_t bg_color;  // 背景颜色 (RGB565)
    uint16_t fg_color;  // 前景颜色 (RGB565)
    float progress;     // 进度值 (0-100)
} progress_ring_t;

/**
 * @brief 初始化进度环
 * @param ring 进度环结构体指针
 */
void progress_ring_init(progress_ring_t *ring);

/**
 * @brief 绘制进度环
 * @param ring 进度环结构体指针
 */
void progress_ring_draw(progress_ring_t *ring);

/**
 * @brief 设置进度值
 * @param ring 进度环结构体指针
 * @param progress 进度值 (0-100)
 */
void progress_ring_set_progress(progress_ring_t *ring, float progress);

#ifdef __cplusplus
}
#endif

#endif // PROGRESS_RING_H
