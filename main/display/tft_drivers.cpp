/**
 * @file tft_drivers.cpp
 * @brief GC9A01 圆形 TFT 显示驱动实现
 *
 * 硬件平台: ESP32-S3 + 240x240 圆形 SPI TFT (GC9A01)
 *
 * 通信接口: SPI mode 0, 40MHz
 * 数据格式: RGB565 (16位)
 *
 * 初始化流程:
 * 1. 配置GPIO (CS, DC, RST, BLK)
 * 2. 初始化SPI总线
 * 3. 添加SPI设备
 * 4. 硬件复位屏幕
 * 5. 发送初始化序列
 * 6. 开启背光
 */

#include "tft_drivers.h"
#include "board_pins.h"
#include "esp_log.h"
#include "esp_delay.h"

// 日志TAG
static const char *TAG = "TFT";

// GC9A01 命令定义
#define GC9A01_SLPIN    0x10    // 睡眠模式进入
#define GC9A01_SLPOUT   0x11    // 睡眠模式退出
#define GC9A01_DISPON   0x29    // 显示开启
#define GC9A01_CASET    0x2A    // 列地址设置
#define GC9A01_RASET    0x2B    // 行地址设置
#define GC9A01_RAMWR    0x2C    // 内存写
#define GC9A01_MADCTL   0x36    // 内存访问控制
#define GC9A01_COLMOD   0x3A    // 颜色模式
#define GC9A01_PORCTRL  0xB2    // 端口设置
#define GC9A01_GCTRL    0xB7    // 门控设置
#define GC9A01_VCOMS    0xBB    // VCOM电压设置
#define GC9A01_LCMCTRL  0xC0    // LCM控制
#define GC9A01_VDVVRHEN 0xC2    // VDV和VRH使能
#define GC9A01_VRHS     0xC3    // VRH电压设置
#define GC9A01_VDVS     0xC4    // VDVS电压设置
#define GC9A01_FRCTRL1  0xC6    // 帧率控制
#define GC9A01_PWCTRL1  0xD0    // 电源控制
#define GC9A01_PGAMCTRL 0xE0    // 正GAMMA设置
#define GC9A01_NGAMCTRL 0xE1    // 负GAMMA设置
#define GC9A01_INVON    0x21    // 显示反色开启

/**
 * @brief SPI传输前回调
 *
 * 根据传输数据类型设置DC引脚:
 * - 命令: DC=0
 * - 数据: DC=1
 */
static void tft_spi_pre_transfer_callback(spi_transaction_t *t) {
    tft_context_t *ctx = (tft_context_t *)t->user;
    gpio_set_level(ctx->dc, (t->flags & SPI_TRANS_USE_TX_DATA) ? 0 : 1);
}

/**
 * @brief 初始化TFT显示屏
 */
int tft_init(tft_context_t *ctx) {
    ESP_LOGI(TAG, "正在初始化GC9A01 TFT显示屏");

    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ctx->cs) | (1ULL << ctx->dc) | (1ULL << ctx->rst) | (1ULL << ctx->blk),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 初始化SPI总线
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = ctx->mosi,
        .miso_io_num = ctx->miso,
        .sclk_io_num = ctx->sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(ctx->spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI总线初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 添加SPI设备
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = ctx->spi_freq,
        .mode = 0,
        .spics_io_num = ctx->cs,
        .queue_size = 7,
        .pre_cb = tft_spi_pre_transfer_callback,
        .user = ctx
    };

    ret = spi_bus_add_device(ctx->spi_host, &dev_cfg, &ctx->spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加SPI设备失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 硬件复位屏幕
    gpio_set_level(ctx->rst, 0);
    esp_rom_delay_us(100000);
    gpio_set_level(ctx->rst, 1);
    esp_rom_delay_us(100000);

    // GC9A01 圆形屏初始化序列
    tft_write_command(ctx, GC9A01_SLPOUT);
    esp_rom_delay_us(120000);

    tft_write_command(ctx, GC9A01_MADCTL);
    tft_write_data(ctx, (uint8_t[]){0x68}, 1);  // MX, MV, RGB

    tft_write_command(ctx, GC9A01_COLMOD);
    tft_write_data(ctx, (uint8_t[]){0x05}, 1);  // 16-bit RGB565

    tft_write_command(ctx, GC9A01_PORCTRL);
    tft_write_data(ctx, (uint8_t[]){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5);

    tft_write_command(ctx, GC9A01_GCTRL);
    tft_write_data(ctx, (uint8_t[]){0x35}, 1);

    tft_write_command(ctx, GC9A01_VCOMS);
    tft_write_data(ctx, (uint8_t[]){0x1B}, 1);

    tft_write_command(ctx, GC9A01_LCMCTRL);
    tft_write_data(ctx, (uint8_t[]){0x2C}, 1);

    tft_write_command(ctx, GC9A01_VDVVRHEN);
    tft_write_data(ctx, (uint8_t[]){0x01}, 1);

    tft_write_command(ctx, GC9A01_VRHS);
    tft_write_data(ctx, (uint8_t[]){0x1C}, 1);

    tft_write_command(ctx, GC9A01_FRCTRL1);
    tft_write_data(ctx, (uint8_t[]){0x01}, 1);

    tft_write_command(ctx, GC9A01_PWCTRL1);
    tft_write_data(ctx, (uint8_t[]){0xA9, 0x10}, 2);

    tft_write_command(ctx, GC9A01_PGAMCTRL);
    tft_write_data(ctx, (uint8_t[]){0xD0, 0x02, 0x02, 0x13, 0x11, 0x2B, 0x3C, 0x44, 0x4C, 0x2D, 0x1F, 0x1F, 0x1F, 0x23}, 14);

    tft_write_command(ctx, GC9A01_NGAMCTRL);
    tft_write_data(ctx, (uint8_t[]){0xD0, 0x02, 0x02, 0x13, 0x11, 0x2B, 0x3C, 0x44, 0x4C, 0x2D, 0x1F, 0x1F, 0x1F, 0x23}, 14);

    tft_write_command(ctx, GC9A01_INVON);
    esp_rom_delay_us(10000);

    tft_write_command(ctx, GC9A01_DISPON);
    esp_rom_delay_us(100000);

    // 开启背光
    gpio_set_level(ctx->blk, 1);

    ESP_LOGI(TAG, "GC9A01 TFT显示屏初始化成功");
    return ESP_OK;
}

/**
 * @brief 发送命令到TFT
 */
void tft_write_command(tft_context_t *ctx, uint8_t cmd) {
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TX_DATA,
        .length = 8,
        .tx_data = {cmd},
        .user = ctx
    };
    spi_device_transmit(ctx->spi, &t);
}

/**
 * @brief 发送数据到TFT
 */
void tft_write_data(tft_context_t *ctx, const uint8_t *data, size_t len) {
    if (len == 0) return;

    if (len <= 4) {
        // 小数据使用内联缓冲区
        spi_transaction_t t = {
            .flags = SPI_TRANS_USE_TX_DATA,
            .length = len * 8,
            .tx_data = {data[0], len > 1 ? data[1] : 0, len > 2 ? data[2] : 0, len > 3 ? data[3] : 0},
            .user = ctx
        };
        spi_device_transmit(ctx->spi, &t);
    } else {
        // 大数据使用外部缓冲区
        spi_transaction_t t = {
            .flags = 0,
            .length = len * 8,
            .tx_buffer = data,
            .user = ctx
        };
        spi_device_transmit(ctx->spi, &t);
    }
}

/**
 * @brief 设置显示窗口
 */
void tft_set_window(tft_context_t *ctx, int x, int y, int w, int h) {
    int xe = x + w - 1;
    int ye = y + h - 1;

    uint8_t caset_data[] = {(x >> 8) & 0xFF, x & 0xFF, (xe >> 8) & 0xFF, xe & 0xFF};
    uint8_t raset_data[] = {(y >> 8) & 0xFF, y & 0xFF, (ye >> 8) & 0xFF, ye & 0xFF};

    tft_write_command(ctx, GC9A01_CASET);
    tft_write_data(ctx, caset_data, 4);

    tft_write_command(ctx, GC9A01_RASET);
    tft_write_data(ctx, raset_data, 4);

    tft_write_command(ctx, GC9A01_RAMWR);
}

/**
 * @brief 画单个像素
 */
void tft_draw_pixel(tft_context_t *ctx, int x, int y, uint16_t color) {
    tft_set_window(ctx, x, y, 1, 1);
    uint8_t color_data[] = {(color >> 8) & 0xFF, color & 0xFF};
    tft_write_data(ctx, color_data, 2);
}

/**
 * @brief 填充矩形区域
 */
void tft_fill_rect(tft_context_t *ctx, int x, int y, int w, int h, uint16_t color) {
    tft_set_window(ctx, x, y, w, h);

    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;

    spi_transaction_t t = {
        .flags = 0,
        .length = w * h * 16,
        .user = ctx
    };

    // 分配像素数据缓冲区
    uint8_t *buf = (uint8_t *)heap_caps_malloc(w * h * 2, MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "fill_rect内存分配失败");
        return;
    }

    // 填充颜色数据
    for (int i = 0; i < w * h * 2; i += 2) {
        buf[i] = color_hi;
        buf[i + 1] = color_lo;
    }

    t.tx_buffer = buf;
    spi_device_transmit(ctx->spi, &t);
    free(buf);
}

/**
 * @brief 清屏
 */
void tft_clear(tft_context_t *ctx, uint16_t color) {
    tft_fill_rect(ctx, 0, 0, ctx->width, ctx->height, color);
}

/**
 * @brief 设置背光
 */
void tft_set_backlight(tft_context_t *ctx, uint8_t brightness) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << ctx->blk,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    if (brightness > 0) {
        gpio_set_level(ctx->blk, 1);
    } else {
        gpio_set_level(ctx->blk, 0);
    }
}
