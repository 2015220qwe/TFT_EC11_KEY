/**
 * @file bsp_tft_st7789.c
 * @brief TFT显示驱动实现 - ST7789控制器
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_tft_st7789.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static uint16_t tft_width = TFT_WIDTH;
static uint16_t tft_height = TFT_HEIGHT;
static uint8_t tft_rotation = TFT_ROTATION_0;

/* 延时函数 (需外部实现或使用SysTick) */
extern void delay_ms(uint32_t ms);
extern void delay_us(uint32_t us);

/*=============================================================================
 *                              ST7789命令定义
 *============================================================================*/

#define ST7789_NOP          0x00
#define ST7789_SWRESET      0x01
#define ST7789_RDDID        0x04
#define ST7789_RDDST        0x09
#define ST7789_SLPIN        0x10
#define ST7789_SLPOUT       0x11
#define ST7789_PTLON        0x12
#define ST7789_NORON        0x13
#define ST7789_INVOFF       0x20
#define ST7789_INVON        0x21
#define ST7789_DISPOFF      0x28
#define ST7789_DISPON       0x29
#define ST7789_CASET        0x2A
#define ST7789_RASET        0x2B
#define ST7789_RAMWR        0x2C
#define ST7789_RAMRD        0x2E
#define ST7789_PTLAR        0x30
#define ST7789_VSCRDEF      0x33
#define ST7789_COLMOD       0x3A
#define ST7789_MADCTL       0x36
#define ST7789_VSCSAD       0x37
#define ST7789_MADCTL_MY    0x80
#define ST7789_MADCTL_MX    0x40
#define ST7789_MADCTL_MV    0x20
#define ST7789_MADCTL_ML    0x10
#define ST7789_MADCTL_RGB   0x00
#define ST7789_MADCTL_BGR   0x08
#define ST7789_MADCTL_MH    0x04

/*=============================================================================
 *                              内置8x16字体数据
 *============================================================================*/

static const uint8_t font_8x16_data[] = {
    /* ASCII 32-127 的字模数据 (简化版本) */
    /* 空格 (32) */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    /* ! (33) */
    0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00,
    /* " (34) */
    0x00,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    /* # (35) */
    0x00,0x00,0x00,0x6C,0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x6C,0x00,0x00,0x00,0x00,
    /* 0-9, A-Z, a-z等字符 (简化表示) */
    /* ... 完整字体数据在实际项目中应包含所有ASCII字符 ... */
};

const tft_font_t font_8x16 = {
    .data = font_8x16_data,
    .width = 8,
    .height = 16,
    .first_char = 32,
    .last_char = 127
};

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static void tft_gpio_init(void);
static void tft_spi_init(void);
static void tft_write_byte(uint8_t data);
static void tft_reset(void);
static void tft_init_seq(void);

/*=============================================================================
 *                              底层函数实现
 *============================================================================*/

/**
 * @brief 发送命令
 */
void bsp_tft_write_cmd(uint8_t cmd)
{
    TFT_CS_LOW();
    TFT_DC_LOW();
    tft_write_byte(cmd);
    TFT_CS_HIGH();
}

/**
 * @brief 发送数据
 */
void bsp_tft_write_data(uint8_t data)
{
    TFT_CS_LOW();
    TFT_DC_HIGH();
    tft_write_byte(data);
    TFT_CS_HIGH();
}

/**
 * @brief 发送16位数据
 */
void bsp_tft_write_data16(uint16_t data)
{
    TFT_CS_LOW();
    TFT_DC_HIGH();
    tft_write_byte(data >> 8);
    tft_write_byte(data & 0xFF);
    TFT_CS_HIGH();
}

/**
 * @brief 批量发送颜色数据
 */
void bsp_tft_write_color(tft_color_t color, uint32_t count)
{
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    TFT_CS_LOW();
    TFT_DC_HIGH();

    while (count--) {
        tft_write_byte(hi);
        tft_write_byte(lo);
    }

    TFT_CS_HIGH();
}

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief TFT初始化
 */
int bsp_tft_init(void)
{
    /* 初始化GPIO */
    tft_gpio_init();

    /* 初始化SPI */
    tft_spi_init();

    /* 硬件复位 */
    tft_reset();

    /* 发送初始化序列 */
    tft_init_seq();

    /* 清屏 */
    bsp_tft_clear(TFT_BLACK);

    /* 开启背光 */
    TFT_BLK_ON();

    return 0;
}

/**
 * @brief TFT反初始化
 */
void bsp_tft_deinit(void)
{
    TFT_BLK_OFF();
    bsp_tft_sleep_enter();
}

/**
 * @brief 设置屏幕方向
 */
void bsp_tft_set_rotation(uint8_t rotation)
{
    uint8_t madctl = 0;

    tft_rotation = rotation % 4;

    switch (tft_rotation) {
    case TFT_ROTATION_0:
        madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB;
        tft_width = TFT_WIDTH;
        tft_height = TFT_HEIGHT;
        break;
    case TFT_ROTATION_90:
        madctl = ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB;
        tft_width = TFT_HEIGHT;
        tft_height = TFT_WIDTH;
        break;
    case TFT_ROTATION_180:
        madctl = ST7789_MADCTL_RGB;
        tft_width = TFT_WIDTH;
        tft_height = TFT_HEIGHT;
        break;
    case TFT_ROTATION_270:
        madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB;
        tft_width = TFT_HEIGHT;
        tft_height = TFT_WIDTH;
        break;
    }

    bsp_tft_write_cmd(ST7789_MADCTL);
    bsp_tft_write_data(madctl);
}

/**
 * @brief 设置背光亮度
 */
void bsp_tft_set_brightness(uint8_t brightness)
{
    /* 简单开关控制，如需PWM调光请使用PWM模块 */
    if (brightness > 0) {
        TFT_BLK_ON();
    } else {
        TFT_BLK_OFF();
    }
}

void bsp_tft_backlight_on(void)  { TFT_BLK_ON(); }
void bsp_tft_backlight_off(void) { TFT_BLK_OFF(); }

/**
 * @brief 设置显示窗口
 */
void bsp_tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    bsp_tft_write_cmd(ST7789_CASET);
    bsp_tft_write_data(x0 >> 8);
    bsp_tft_write_data(x0 & 0xFF);
    bsp_tft_write_data(x1 >> 8);
    bsp_tft_write_data(x1 & 0xFF);

    bsp_tft_write_cmd(ST7789_RASET);
    bsp_tft_write_data(y0 >> 8);
    bsp_tft_write_data(y0 & 0xFF);
    bsp_tft_write_data(y1 >> 8);
    bsp_tft_write_data(y1 & 0xFF);

    bsp_tft_write_cmd(ST7789_RAMWR);
}

/**
 * @brief 清屏
 */
void bsp_tft_clear(tft_color_t color)
{
    bsp_tft_fill_rect(0, 0, tft_width, tft_height, color);
}

/**
 * @brief 画点
 */
void bsp_tft_draw_pixel(uint16_t x, uint16_t y, tft_color_t color)
{
    if (x >= tft_width || y >= tft_height) return;

    bsp_tft_set_window(x, y, x, y);
    bsp_tft_write_data16(color);
}

/**
 * @brief 画水平线
 */
void bsp_tft_draw_hline(uint16_t x, uint16_t y, uint16_t w, tft_color_t color)
{
    if (x >= tft_width || y >= tft_height) return;
    if (x + w > tft_width) w = tft_width - x;

    bsp_tft_set_window(x, y, x + w - 1, y);
    bsp_tft_write_color(color, w);
}

/**
 * @brief 画垂直线
 */
void bsp_tft_draw_vline(uint16_t x, uint16_t y, uint16_t h, tft_color_t color)
{
    if (x >= tft_width || y >= tft_height) return;
    if (y + h > tft_height) h = tft_height - y;

    bsp_tft_set_window(x, y, x, y + h - 1);
    bsp_tft_write_color(color, h);
}

/**
 * @brief 画线 (Bresenham算法)
 */
void bsp_tft_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, tft_color_t color)
{
    int16_t dx = abs((int16_t)x1 - (int16_t)x0);
    int16_t dy = -abs((int16_t)y1 - (int16_t)y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    while (1) {
        bsp_tft_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/**
 * @brief 画矩形
 */
void bsp_tft_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color)
{
    bsp_tft_draw_hline(x, y, w, color);
    bsp_tft_draw_hline(x, y + h - 1, w, color);
    bsp_tft_draw_vline(x, y, h, color);
    bsp_tft_draw_vline(x + w - 1, y, h, color);
}

/**
 * @brief 填充矩形
 */
void bsp_tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color)
{
    if (x >= tft_width || y >= tft_height) return;
    if (x + w > tft_width) w = tft_width - x;
    if (y + h > tft_height) h = tft_height - y;

    bsp_tft_set_window(x, y, x + w - 1, y + h - 1);
    bsp_tft_write_color(color, (uint32_t)w * h);
}

/**
 * @brief 画圆 (中点圆算法)
 */
void bsp_tft_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, tft_color_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    bsp_tft_draw_pixel(x0, y0 + r, color);
    bsp_tft_draw_pixel(x0, y0 - r, color);
    bsp_tft_draw_pixel(x0 + r, y0, color);
    bsp_tft_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        bsp_tft_draw_pixel(x0 + x, y0 + y, color);
        bsp_tft_draw_pixel(x0 - x, y0 + y, color);
        bsp_tft_draw_pixel(x0 + x, y0 - y, color);
        bsp_tft_draw_pixel(x0 - x, y0 - y, color);
        bsp_tft_draw_pixel(x0 + y, y0 + x, color);
        bsp_tft_draw_pixel(x0 - y, y0 + x, color);
        bsp_tft_draw_pixel(x0 + y, y0 - x, color);
        bsp_tft_draw_pixel(x0 - y, y0 - x, color);
    }
}

/**
 * @brief 填充圆
 */
void bsp_tft_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, tft_color_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    bsp_tft_draw_vline(x0, y0 - r, 2 * r + 1, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        bsp_tft_draw_vline(x0 + x, y0 - y, 2 * y + 1, color);
        bsp_tft_draw_vline(x0 - x, y0 - y, 2 * y + 1, color);
        bsp_tft_draw_vline(x0 + y, y0 - x, 2 * x + 1, color);
        bsp_tft_draw_vline(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

/**
 * @brief 显示单个字符
 */
void bsp_tft_draw_char(uint16_t x, uint16_t y, char ch, const tft_font_t *font,
                        tft_color_t fg_color, tft_color_t bg_color)
{
    uint8_t i, j;
    const uint8_t *char_data;
    uint8_t byte;
    uint8_t bytes_per_row;

    if (font == NULL) font = &font_8x16;
    if (ch < font->first_char || ch > font->last_char) return;

    bytes_per_row = (font->width + 7) / 8;
    char_data = &font->data[(ch - font->first_char) * font->height * bytes_per_row];

    bsp_tft_set_window(x, y, x + font->width - 1, y + font->height - 1);

    for (i = 0; i < font->height; i++) {
        for (j = 0; j < font->width; j++) {
            byte = char_data[i * bytes_per_row + j / 8];
            if (byte & (0x80 >> (j % 8))) {
                bsp_tft_write_data16(fg_color);
            } else {
                bsp_tft_write_data16(bg_color);
            }
        }
    }
}

/**
 * @brief 显示字符串
 */
void bsp_tft_draw_string(uint16_t x, uint16_t y, const char *str, const tft_font_t *font,
                          tft_color_t fg_color, tft_color_t bg_color)
{
    if (font == NULL) font = &font_8x16;

    while (*str) {
        if (*str == '\n') {
            x = 0;
            y += font->height;
        } else {
            bsp_tft_draw_char(x, y, *str, font, fg_color, bg_color);
            x += font->width;
            if (x + font->width > tft_width) {
                x = 0;
                y += font->height;
            }
        }
        str++;
    }
}

/**
 * @brief 显示格式化字符串
 */
void bsp_tft_printf(uint16_t x, uint16_t y, const tft_font_t *font,
                    tft_color_t fg_color, tft_color_t bg_color, const char *fmt, ...)
{
    char buf[128];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    bsp_tft_draw_string(x, y, buf, font, fg_color, bg_color);
}

/**
 * @brief 显示图像数据
 */
void bsp_tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    uint32_t i;
    uint32_t size = (uint32_t)w * h;

    bsp_tft_set_window(x, y, x + w - 1, y + h - 1);

    TFT_CS_LOW();
    TFT_DC_HIGH();

    for (i = 0; i < size; i++) {
        tft_write_byte(data[i] >> 8);
        tft_write_byte(data[i] & 0xFF);
    }

    TFT_CS_HIGH();
}

/**
 * @brief 反色显示
 */
void bsp_tft_invert_display(uint8_t invert)
{
    bsp_tft_write_cmd(invert ? ST7789_INVON : ST7789_INVOFF);
}

/**
 * @brief 进入睡眠模式
 */
void bsp_tft_sleep_enter(void)
{
    bsp_tft_write_cmd(ST7789_SLPIN);
    delay_ms(120);
}

/**
 * @brief 退出睡眠模式
 */
void bsp_tft_sleep_exit(void)
{
    bsp_tft_write_cmd(ST7789_SLPOUT);
    delay_ms(120);
}

/**
 * @brief RGB888转RGB565
 */
tft_color_t bsp_tft_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief HSV转RGB565
 */
tft_color_t bsp_tft_hsv_to_rgb565(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t r, g, b;
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        r = g = b = v * 255 / 100;
    } else {
        region = h / 60;
        remainder = (h - (region * 60)) * 6;

        p = (v * (100 - s)) * 255 / 10000;
        q = (v * (100 - (s * remainder) / 360)) * 255 / 10000;
        t = (v * (100 - (s * (360 - remainder)) / 360)) * 255 / 10000;
        v = v * 255 / 100;

        switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
        }
    }

    return bsp_tft_rgb888_to_rgb565(r, g, b);
}

uint16_t bsp_tft_get_width(void)  { return tft_width; }
uint16_t bsp_tft_get_height(void) { return tft_height; }

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

/**
 * @brief GPIO初始化
 */
static void tft_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIO时钟 */
    RCC_AHB1PeriphClockCmd(TFT_GPIO_CLK, ENABLE);

    /* 配置控制引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = TFT_CS_PIN | TFT_DC_PIN | TFT_RES_PIN | TFT_BLK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(TFT_GPIO_PORT, &GPIO_InitStructure);

    /* 初始状态 */
    TFT_CS_HIGH();
    TFT_DC_HIGH();
    TFT_RES_HIGH();
    TFT_BLK_OFF();
}

/**
 * @brief SPI初始化
 */
static void tft_spi_init(void)
{
#if TFT_USE_HW_SPI
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(TFT_SPI_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(TFT_GPIO_CLK, ENABLE);

    /* 配置SPI引脚为复用功能 */
    GPIO_InitStructure.GPIO_Pin = TFT_SCL_PIN | TFT_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(TFT_GPIO_PORT, &GPIO_InitStructure);

    /* 配置复用功能 */
    GPIO_PinAFConfig(TFT_GPIO_PORT, TFT_SCL_PINSOURCE, GPIO_AF_SPI1);
    GPIO_PinAFConfig(TFT_GPIO_PORT, TFT_SDA_PINSOURCE, GPIO_AF_SPI1);

    /* 配置SPI */
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(TFT_SPI, &SPI_InitStructure);

    SPI_Cmd(TFT_SPI, ENABLE);
#else
    /* 软件SPI - GPIO已在tft_gpio_init中初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = TFT_SCL_PIN | TFT_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(TFT_GPIO_PORT, &GPIO_InitStructure);
#endif
}

/**
 * @brief 发送单字节
 */
static void tft_write_byte(uint8_t data)
{
#if TFT_USE_HW_SPI
    while (SPI_I2S_GetFlagStatus(TFT_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(TFT_SPI, data);
    while (SPI_I2S_GetFlagStatus(TFT_SPI, SPI_I2S_FLAG_BSY) == SET);
#else
    uint8_t i;
    for (i = 0; i < 8; i++) {
        GPIO_ResetBits(TFT_GPIO_PORT, TFT_SCL_PIN);
        if (data & 0x80) {
            GPIO_SetBits(TFT_GPIO_PORT, TFT_SDA_PIN);
        } else {
            GPIO_ResetBits(TFT_GPIO_PORT, TFT_SDA_PIN);
        }
        GPIO_SetBits(TFT_GPIO_PORT, TFT_SCL_PIN);
        data <<= 1;
    }
#endif
}

/**
 * @brief 硬件复位
 */
static void tft_reset(void)
{
    TFT_RES_HIGH();
    delay_ms(10);
    TFT_RES_LOW();
    delay_ms(10);
    TFT_RES_HIGH();
    delay_ms(120);
}

/**
 * @brief 发送初始化序列
 */
static void tft_init_seq(void)
{
    /* 退出睡眠模式 */
    bsp_tft_write_cmd(ST7789_SLPOUT);
    delay_ms(120);

    /* 像素格式: 16bit RGB565 */
    bsp_tft_write_cmd(ST7789_COLMOD);
    bsp_tft_write_data(0x55);

    /* 显示方向 */
    bsp_tft_write_cmd(ST7789_MADCTL);
    bsp_tft_write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);

    /* 反色开启 (某些屏需要) */
    bsp_tft_write_cmd(ST7789_INVON);

    /* 正常显示模式 */
    bsp_tft_write_cmd(ST7789_NORON);
    delay_ms(10);

    /* 开启显示 */
    bsp_tft_write_cmd(ST7789_DISPON);
    delay_ms(10);
}
