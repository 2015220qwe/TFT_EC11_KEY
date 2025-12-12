/**
 * @file bsp_tft_hal.c
 * @brief TFT显示驱动 - HAL库版本实现
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#ifdef USE_HAL_DRIVER

#include "bsp_hal/bsp_tft_hal.h"
#include <string.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static SPI_HandleTypeDef *hspi_instance = NULL;
static GPIO_TypeDef *cs_gpio_port = NULL;
static uint16_t cs_gpio_pin = 0;
static GPIO_TypeDef *dc_gpio_port = NULL;
static uint16_t dc_gpio_pin = 0;
static GPIO_TypeDef *rst_gpio_port = NULL;
static uint16_t rst_gpio_pin = 0;

static uint8_t tft_rotation = 0;

/*=============================================================================
 *                              私有函数
 *============================================================================*/

#define TFT_CS_LOW()    HAL_GPIO_WritePin(cs_gpio_port, cs_gpio_pin, GPIO_PIN_RESET)
#define TFT_CS_HIGH()   HAL_GPIO_WritePin(cs_gpio_port, cs_gpio_pin, GPIO_PIN_SET)
#define TFT_DC_LOW()    HAL_GPIO_WritePin(dc_gpio_port, dc_gpio_pin, GPIO_PIN_RESET)
#define TFT_DC_HIGH()   HAL_GPIO_WritePin(dc_gpio_port, dc_gpio_pin, GPIO_PIN_SET)
#define TFT_RST_LOW()   HAL_GPIO_WritePin(rst_gpio_port, rst_gpio_pin, GPIO_PIN_RESET)
#define TFT_RST_HIGH()  HAL_GPIO_WritePin(rst_gpio_port, rst_gpio_pin, GPIO_PIN_SET)

static void tft_write_cmd(uint8_t cmd)
{
    TFT_DC_LOW();
    TFT_CS_LOW();
    HAL_SPI_Transmit(hspi_instance, &cmd, 1, 100);
    TFT_CS_HIGH();
}

static void tft_write_data(uint8_t data)
{
    TFT_DC_HIGH();
    TFT_CS_LOW();
    HAL_SPI_Transmit(hspi_instance, &data, 1, 100);
    TFT_CS_HIGH();
}

static void tft_write_data16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;

    TFT_DC_HIGH();
    TFT_CS_LOW();
    HAL_SPI_Transmit(hspi_instance, buf, 2, 100);
    TFT_CS_HIGH();
}

static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_write_cmd(0x2A);
    tft_write_data(x0 >> 8);
    tft_write_data(x0 & 0xFF);
    tft_write_data(x1 >> 8);
    tft_write_data(x1 & 0xFF);

    tft_write_cmd(0x2B);
    tft_write_data(y0 >> 8);
    tft_write_data(y0 & 0xFF);
    tft_write_data(y1 >> 8);
    tft_write_data(y1 & 0xFF);

    tft_write_cmd(0x2C);
}

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief TFT初始化
 */
int bsp_tft_hal_init(SPI_HandleTypeDef *hspi,
                     GPIO_TypeDef *cs_port, uint16_t cs_pin,
                     GPIO_TypeDef *dc_port, uint16_t dc_pin,
                     GPIO_TypeDef *rst_port, uint16_t rst_pin)
{
    if (hspi == NULL) {
        return -1;
    }

    hspi_instance = hspi;
    cs_gpio_port = cs_port;
    cs_gpio_pin = cs_pin;
    dc_gpio_port = dc_port;
    dc_gpio_pin = dc_pin;
    rst_gpio_port = rst_port;
    rst_gpio_pin = rst_pin;

    /* 复位 */
    TFT_RST_HIGH();
    HAL_Delay(10);
    TFT_RST_LOW();
    HAL_Delay(10);
    TFT_RST_HIGH();
    HAL_Delay(120);

    /* ST7789初始化序列 */
    tft_write_cmd(0x11);    /* Sleep Out */
    HAL_Delay(120);

    tft_write_cmd(0x36);    /* Memory Access Control */
    tft_write_data(0x00);

    tft_write_cmd(0x3A);    /* Pixel Format */
    tft_write_data(0x55);   /* RGB565 */

    tft_write_cmd(0xB2);    /* Porch Setting */
    tft_write_data(0x0C);
    tft_write_data(0x0C);
    tft_write_data(0x00);
    tft_write_data(0x33);
    tft_write_data(0x33);

    tft_write_cmd(0xB7);    /* Gate Control */
    tft_write_data(0x35);

    tft_write_cmd(0xBB);    /* VCOM Setting */
    tft_write_data(0x19);

    tft_write_cmd(0xC0);    /* LCM Control */
    tft_write_data(0x2C);

    tft_write_cmd(0xC2);    /* VDV and VRH Command Enable */
    tft_write_data(0x01);

    tft_write_cmd(0xC3);    /* VRH Set */
    tft_write_data(0x12);

    tft_write_cmd(0xC4);    /* VDV Set */
    tft_write_data(0x20);

    tft_write_cmd(0xC6);    /* Frame Rate Control */
    tft_write_data(0x0F);

    tft_write_cmd(0xD0);    /* Power Control 1 */
    tft_write_data(0xA4);
    tft_write_data(0xA1);

    tft_write_cmd(0x21);    /* Display Inversion On */

    tft_write_cmd(0x29);    /* Display On */

    bsp_tft_hal_clear(TFT_BLACK);

    return 0;
}

/**
 * @brief 清屏
 */
void bsp_tft_hal_clear(tft_color_t color)
{
    bsp_tft_hal_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

/**
 * @brief 画点
 */
void bsp_tft_hal_draw_pixel(uint16_t x, uint16_t y, tft_color_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    tft_set_window(x, y, x, y);
    tft_write_data16(color);
}

/**
 * @brief 填充矩形
 */
void bsp_tft_hal_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color)
{
    uint32_t i;
    uint8_t data[2];

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    if (x + w > TFT_WIDTH) w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    tft_set_window(x, y, x + w - 1, y + h - 1);

    data[0] = color >> 8;
    data[1] = color & 0xFF;

    TFT_DC_HIGH();
    TFT_CS_LOW();

    for (i = 0; i < (uint32_t)w * h; i++) {
        HAL_SPI_Transmit(hspi_instance, data, 2, 100);
    }

    TFT_CS_HIGH();
}

/**
 * @brief 画线
 */
void bsp_tft_hal_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, tft_color_t color)
{
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int16_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;
    int16_t e2;

    while (1) {
        bsp_tft_hal_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief 显示字符串 (简化版，使用内置5x7字体)
 */
void bsp_tft_hal_draw_string(uint16_t x, uint16_t y, const char *str, tft_color_t fg, tft_color_t bg)
{
    /* 简化的5x7 ASCII字体 */
    static const uint8_t font5x7[][5] = {
        {0x00,0x00,0x00,0x00,0x00}, /* Space */
        {0x00,0x00,0x5F,0x00,0x00}, /* ! */
        /* ... 更多字符 */
    };

    /* 简化实现：仅显示占位 */
    uint16_t offset = 0;
    while (*str) {
        /* 画一个简单的字符框 */
        bsp_tft_hal_fill_rect(x + offset, y, 6, 8, bg);
        offset += 6;
        str++;
    }
}

/**
 * @brief 显示图像
 */
void bsp_tft_hal_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    uint32_t i;

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || data == NULL) {
        return;
    }

    tft_set_window(x, y, x + w - 1, y + h - 1);

    TFT_DC_HIGH();
    TFT_CS_LOW();

    for (i = 0; i < (uint32_t)w * h; i++) {
        uint8_t buf[2];
        buf[0] = data[i] >> 8;
        buf[1] = data[i] & 0xFF;
        HAL_SPI_Transmit(hspi_instance, buf, 2, 100);
    }

    TFT_CS_HIGH();
}

/**
 * @brief 设置屏幕方向
 */
void bsp_tft_hal_set_rotation(uint8_t rotation)
{
    uint8_t madctl = 0;

    tft_rotation = rotation % 4;

    switch (tft_rotation) {
    case 0:
        madctl = 0x00;
        break;
    case 1:
        madctl = 0x60;
        break;
    case 2:
        madctl = 0xC0;
        break;
    case 3:
        madctl = 0xA0;
        break;
    }

    tft_write_cmd(0x36);
    tft_write_data(madctl);
}

/**
 * @brief DMA传输
 */
int bsp_tft_hal_dma_transfer(const uint16_t *data, uint32_t len)
{
    if (hspi_instance == NULL || data == NULL || len == 0) {
        return -1;
    }

    TFT_DC_HIGH();
    TFT_CS_LOW();

    if (HAL_SPI_Transmit_DMA(hspi_instance, (uint8_t *)data, len * 2) != HAL_OK) {
        TFT_CS_HIGH();
        return -1;
    }

    return 0;
}

#endif /* USE_HAL_DRIVER */
