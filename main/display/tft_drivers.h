/**
 * @file tft_drivers.h
 * @brief GC9A01 圆形 TFT 显示驱动接口
 *
 * 硬件平台: ESP32-S3 + 240x240 圆形 SPI TFT (GC9A01)
 *
 * 通信接口: SPI (Motorola格式)
 * 数据格式: RGB565 (16位)
 */

#ifndef TFT_DRIVERS_H
#define TFT_DRIVERS_H

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TFT上下文结构体
 */
typedef struct {
    gpio_num_t cs;           // 片选引脚
    gpio_num_t dc;           // 数据/命令引脚
    gpio_num_t rst;          // 复位引脚
    gpio_num_t blk;          // 背光控制引脚
    gpio_num_t mosi;         // SPI MOSI
    gpio_num_t miso;         // SPI MISO (未用)
    gpio_num_t sck;          // SPI时钟
    int width;               // 屏幕宽度
    int height;              // 屏幕高度
    int spi_host;           // SPI主机号
    uint32_t spi_freq;       // SPI频率
    spi_device_handle_t spi; // SPI设备句柄
} tft_context_t;

/**
 * @brief 初始化TFT显示屏
 * @param ctx TFT上下文
 * @return ESP_OK 成功
 */
int tft_init(tft_context_t *ctx);

/**
 * @brief 发送命令到TFT
 * @param ctx TFT上下文
 * @param cmd 命令字节
 */
void tft_write_command(tft_context_t *ctx, uint8_t cmd);

/**
 * @brief 发送数据到TFT
 * @param ctx TFT上下文
 * @param data 数据指针
 * @param len 数据长度
 */
void tft_write_data(tft_context_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief 设置显示窗口
 * @param ctx TFT上下文
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param w 宽度
 * @param h 高度
 */
void tft_set_window(tft_context_t *ctx, int x, int y, int w, int h);

/**
 * @brief 画单个像素
 * @param ctx TFT上下文
 * @param x X坐标
 * @param y Y坐标
 * @param color RGB565颜色值
 */
void tft_draw_pixel(tft_context_t *ctx, int x, int y, uint16_t color);

/**
 * @brief 填充矩形区域
 * @param ctx TFT上下文
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param w 宽度
 * @param h 高度
 * @param color RGB565颜色值
 */
void tft_fill_rect(tft_context_t *ctx, int x, int y, int w, int h, uint16_t color);

/**
 * @brief 清屏
 * @param ctx TFT上下文
 * @param color 填充颜色
 */
void tft_clear(tft_context_t *ctx, uint16_t color);

/**
 * @brief 设置背光亮度
 * @param ctx TFT上下文
 * @param brightness 亮度值 (0-255)
 */
void tft_set_backlight(tft_context_t *ctx, uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // TFT_DRIVERS_H
