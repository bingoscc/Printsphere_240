/**
 * @file board_pins.h
 * @brief genshin_godeye 硬件引脚定义
 *
 * 硬件平台: ESP32-S3 + 240x240 SPI TFT (GC9A01/方形ST7789)
 * 基于 planevina 开源项目设计
 *
 * 引脚分配说明:
 * - TFT屏幕: SPI2总线 (VSPI)
 * - 按键: GPIO38 (支持中断唤醒)
 * - SD卡: SDMMC接口 (用于存储配置和日志)
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "driver/gpio.h"

/**
 * @brief TFT Display SPI接口引脚
 * 硬件连接说明:
 * - GPIO11: TFT片选 (CS)
 * - GPIO12: SPI时钟 (SCK)
 * - GPIO13: SPI数据输出 (MOSI)
 * - GPIO10: 数据/命令切换 (DC)
 * - GPIO21: 复位引脚 (RST)
 * - GPIO14: 背光控制 (BLK)
 */
constexpr gpio_num_t PIN_TFT_CS   = GPIO_NUM_11;   // TFT片选
constexpr gpio_num_t PIN_TFT_SCK  = GPIO_NUM_12;   // SPI时钟
constexpr gpio_num_t PIN_TFT_MOSI = GPIO_NUM_13;   // SPI数据 MOSI
constexpr gpio_num_t PIN_TFT_MISO = GPIO_NUM_NC;   // 未连接 (单向SPI)
constexpr gpio_num_t PIN_TFT_DC   = GPIO_NUM_10;   // 数据/命令引脚
constexpr gpio_num_t PIN_TFT_RST  = GPIO_NUM_21;   // 复位引脚
constexpr gpio_num_t PIN_TFT_BLK  = GPIO_NUM_14;   // 背光控制

/**
 * @brief 按键引脚
 * GPIO38: 连接到物理按键，用于UI交互
 */
constexpr gpio_num_t PIN_BTN      = GPIO_NUM_38;   // 物理按键输入

/**
 * @brief SDMMC SD卡接口引脚
 * 用于存储配置文件、固件更新等
 */
constexpr gpio_num_t PIN_SD_CMD   = GPIO_NUM_42;   // SDMMC命令
constexpr gpio_num_t PIN_SD_CLK   = GPIO_NUM_41;   // SDMMC时钟
constexpr gpio_num_t PIN_SD_D0    = GPIO_NUM_40;   // SDMMC数据0

/**
 * @brief 显示屏配置参数
 */
constexpr int TFT_WIDTH  = 240;              // 屏幕宽度 240px
constexpr int TFT_HEIGHT = 240;              // 屏幕高度 240px
constexpr int TFT_SPI_HOST = SPI2_HOST;      // 使用SPI2主机 (VSPI)
constexpr uint32_t TFT_SPI_FREQ = 40000000;  // SPI时钟 40MHz

#endif // BOARD_PINS_H
