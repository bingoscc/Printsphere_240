/**
 * @file progress_ring.cpp
 * @brief 圆形进度环实现
 *
 * 使用极坐标绘制圆环:
 * - 外半径: 75px
 * - 环宽度: 12px
 * - 圆心: (120, 105)
 *
 * 绘制算法:
 * 1. 遍历0-360度
 * 2. 在每个角度计算外弧和内弧上的点
 * 3. 取中点绘制像素形成圆环
 */

#include "progress_ring.h"
#include "tft_drivers.h"
#include "math.h"

#ifndef PI
#define PI 3.14159265359f
#endif

#ifndef M_2PI
#define M_2PI 6.28318530718f
#endif

/**
 * @brief 初始化进度环
 */
void progress_ring_init(progress_ring_t *ring) {
    ring->x = 120;
    ring->y = 105;
    ring->radius = 75;
    ring->thickness = 12;
    ring->bg_color = 0x1111;  // 深蓝灰
    ring->fg_color = 0xF800;  // 红色
    ring->progress = 0;
}

/**
 * @brief RGB565颜色转换
 */
static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief 绘制进度环
 *
 * @param ring 进度环配置
 */
void progress_ring_draw(progress_ring_t *ring) {
    int x1, y1, x2, y2;
    float angle;
    float sin_a, cos_a;
    int inner_radius = ring->radius - ring->thickness;

    // 绘制背景圆环 (完整圆)
    for (angle = 0; angle < M_2PI; angle += 0.05f) {
        sin_a = sinf(angle);
        cos_a = cosf(angle);

        x1 = ring->x + (int)(ring->radius * cos_a);
        y1 = ring->y + (int)(ring->radius * sin_a);
        x2 = ring->x + (int)(inner_radius * cos_a);
        y2 = ring->y + (int)(inner_radius * sin_a);

        // 简化为只画中点像素
        int mid_x = ring->x + (int)((ring->radius + inner_radius) / 2 * cos_a);
        int mid_y = ring->y + (int)((ring->radius + inner_radius) / 2 * sin_a);
        tft_draw_pixel(mid_x, mid_y, ring->bg_color);
    }

    // 绘制进度圆弧 (从顶部开始顺时针)
    float progress_angle = (ring->progress / 100.0f) * M_2PI;
    for (angle = -PI/2; angle < -PI/2 + progress_angle; angle += 0.05f) {
        sin_a = sinf(angle);
        cos_a = cosf(angle);

        int mid_x = ring->x + (int)((ring->radius + inner_radius) / 2 * cos_a);
        int mid_y = ring->y + (int)((ring->radius + inner_radius) / 2 * sin_a);
        tft_draw_pixel(mid_x, mid_y, ring->fg_color);
    }

    // 百分比文字由LVGL单独处理
}

/**
 * @brief 设置进度值
 *
 * @param ring 进度环
 * @param progress 进度值 (0-100)
 */
void progress_ring_set_progress(progress_ring_t *ring, float progress) {
    // 限制范围在0-100
    ring->progress = progress > 100 ? 100 : (progress < 0 ? 0 : progress);
}
