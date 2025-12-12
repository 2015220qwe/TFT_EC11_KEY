/**
 * @file bsp_oled_hal.c
 * @brief SSD1306 OLED驱动 - HAL库版本实现
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#ifdef USE_HAL_DRIVER

#include "bsp_hal/bsp_oled_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static I2C_HandleTypeDef *hi2c_instance = NULL;
static uint8_t oled_buffer[OLED_HAL_WIDTH * 8];  /* 最大128x64 */
static uint8_t current_height = OLED_HAL_HEIGHT_64;
static uint8_t current_pages = 8;

/* 简化6x8字体 */
static const uint8_t font6x8[] = {
    0x00,0x00,0x00,0x00,0x00,0x00, /* space */
    0x00,0x00,0x5F,0x00,0x00,0x00, /* ! */
    0x00,0x07,0x00,0x07,0x00,0x00, /* " */
    0x14,0x7F,0x14,0x7F,0x14,0x00, /* # */
    0x24,0x2A,0x7F,0x2A,0x12,0x00, /* $ */
    0x23,0x13,0x08,0x64,0x62,0x00, /* % */
    0x36,0x49,0x55,0x22,0x50,0x00, /* & */
    0x00,0x05,0x03,0x00,0x00,0x00, /* ' */
    0x00,0x1C,0x22,0x41,0x00,0x00, /* ( */
    0x00,0x41,0x22,0x1C,0x00,0x00, /* ) */
    0x08,0x2A,0x1C,0x2A,0x08,0x00, /* * */
    0x08,0x08,0x3E,0x08,0x08,0x00, /* + */
    0x00,0x50,0x30,0x00,0x00,0x00, /* , */
    0x08,0x08,0x08,0x08,0x08,0x00, /* - */
    0x00,0x60,0x60,0x00,0x00,0x00, /* . */
    0x20,0x10,0x08,0x04,0x02,0x00, /* / */
    0x3E,0x51,0x49,0x45,0x3E,0x00, /* 0 */
    0x00,0x42,0x7F,0x40,0x00,0x00, /* 1 */
    0x42,0x61,0x51,0x49,0x46,0x00, /* 2 */
    0x21,0x41,0x45,0x4B,0x31,0x00, /* 3 */
    0x18,0x14,0x12,0x7F,0x10,0x00, /* 4 */
    0x27,0x45,0x45,0x45,0x39,0x00, /* 5 */
    0x3C,0x4A,0x49,0x49,0x30,0x00, /* 6 */
    0x01,0x71,0x09,0x05,0x03,0x00, /* 7 */
    0x36,0x49,0x49,0x49,0x36,0x00, /* 8 */
    0x06,0x49,0x49,0x29,0x1E,0x00, /* 9 */
    0x00,0x36,0x36,0x00,0x00,0x00, /* : */
    0x00,0x56,0x36,0x00,0x00,0x00, /* ; */
    0x00,0x08,0x14,0x22,0x41,0x00, /* < */
    0x14,0x14,0x14,0x14,0x14,0x00, /* = */
    0x41,0x22,0x14,0x08,0x00,0x00, /* > */
    0x02,0x01,0x51,0x09,0x06,0x00, /* ? */
    0x32,0x49,0x79,0x41,0x3E,0x00, /* @ */
    0x7E,0x11,0x11,0x11,0x7E,0x00, /* A */
    0x7F,0x49,0x49,0x49,0x36,0x00, /* B */
    0x3E,0x41,0x41,0x41,0x22,0x00, /* C */
    0x7F,0x41,0x41,0x22,0x1C,0x00, /* D */
    0x7F,0x49,0x49,0x49,0x41,0x00, /* E */
    0x7F,0x09,0x09,0x01,0x01,0x00, /* F */
    0x3E,0x41,0x41,0x51,0x32,0x00, /* G */
    0x7F,0x08,0x08,0x08,0x7F,0x00, /* H */
    0x00,0x41,0x7F,0x41,0x00,0x00, /* I */
    0x20,0x40,0x41,0x3F,0x01,0x00, /* J */
    0x7F,0x08,0x14,0x22,0x41,0x00, /* K */
    0x7F,0x40,0x40,0x40,0x40,0x00, /* L */
    0x7F,0x02,0x04,0x02,0x7F,0x00, /* M */
    0x7F,0x04,0x08,0x10,0x7F,0x00, /* N */
    0x3E,0x41,0x41,0x41,0x3E,0x00, /* O */
    0x7F,0x09,0x09,0x09,0x06,0x00, /* P */
    0x3E,0x41,0x51,0x21,0x5E,0x00, /* Q */
    0x7F,0x09,0x19,0x29,0x46,0x00, /* R */
    0x46,0x49,0x49,0x49,0x31,0x00, /* S */
    0x01,0x01,0x7F,0x01,0x01,0x00, /* T */
    0x3F,0x40,0x40,0x40,0x3F,0x00, /* U */
    0x1F,0x20,0x40,0x20,0x1F,0x00, /* V */
    0x7F,0x20,0x18,0x20,0x7F,0x00, /* W */
    0x63,0x14,0x08,0x14,0x63,0x00, /* X */
    0x03,0x04,0x78,0x04,0x03,0x00, /* Y */
    0x61,0x51,0x49,0x45,0x43,0x00, /* Z */
};

/*=============================================================================
 *                              私有函数
 *============================================================================*/

static void oled_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(hi2c_instance, OLED_HAL_I2C_ADDR, data, 2, 100);
}

static void oled_write_data(uint8_t *data, uint16_t len)
{
    uint8_t buf[129];
    buf[0] = 0x40;
    memcpy(&buf[1], data, len > 128 ? 128 : len);
    HAL_I2C_Master_Transmit(hi2c_instance, OLED_HAL_I2C_ADDR, buf, len + 1, 100);
}

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

int bsp_oled_hal_init(I2C_HandleTypeDef *hi2c, oled_hal_type_t type)
{
    if (hi2c == NULL) {
        return -1;
    }

    hi2c_instance = hi2c;

    if (type == OLED_HAL_TYPE_128X32) {
        current_height = OLED_HAL_HEIGHT_32;
        current_pages = 4;
    } else {
        current_height = OLED_HAL_HEIGHT_64;
        current_pages = 8;
    }

    HAL_Delay(100);

    /* 初始化序列 */
    oled_write_cmd(0xAE); /* Display off */
    oled_write_cmd(0x00); /* Low column */
    oled_write_cmd(0x10); /* High column */
    oled_write_cmd(0x40); /* Start line */
    oled_write_cmd(0xB0); /* Page address */
    oled_write_cmd(0x81); /* Contrast */
    oled_write_cmd(0xCF);
    oled_write_cmd(0xA1); /* Segment remap */
    oled_write_cmd(0xA6); /* Normal display */
    oled_write_cmd(0xA8); /* Multiplex ratio */
    oled_write_cmd(current_height - 1);
    oled_write_cmd(0xC8); /* COM scan direction */
    oled_write_cmd(0xD3); /* Display offset */
    oled_write_cmd(0x00);
    oled_write_cmd(0xD5); /* Clock divide */
    oled_write_cmd(0x80);
    oled_write_cmd(0xD9); /* Pre-charge */
    oled_write_cmd(0xF1);
    oled_write_cmd(0xDA); /* COM pins */
    oled_write_cmd(type == OLED_HAL_TYPE_128X32 ? 0x02 : 0x12);
    oled_write_cmd(0xDB); /* VCOMH */
    oled_write_cmd(0x40);
    oled_write_cmd(0x8D); /* Charge pump */
    oled_write_cmd(0x14);
    oled_write_cmd(0xAF); /* Display on */

    bsp_oled_hal_clear();
    bsp_oled_hal_refresh();

    return 0;
}

void bsp_oled_hal_refresh(void)
{
    uint8_t page;
    for (page = 0; page < current_pages; page++) {
        oled_write_cmd(0xB0 + page);
        oled_write_cmd(0x00);
        oled_write_cmd(0x10);
        oled_write_data(&oled_buffer[OLED_HAL_WIDTH * page], OLED_HAL_WIDTH);
    }
}

void bsp_oled_hal_clear(void)
{
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void bsp_oled_hal_fill(oled_hal_color_t color)
{
    memset(oled_buffer, color ? 0xFF : 0x00, OLED_HAL_WIDTH * current_pages);
}

void bsp_oled_hal_draw_pixel(uint8_t x, uint8_t y, oled_hal_color_t color)
{
    if (x >= OLED_HAL_WIDTH || y >= current_height) return;

    if (color) {
        oled_buffer[x + (y / 8) * OLED_HAL_WIDTH] |= (1 << (y % 8));
    } else {
        oled_buffer[x + (y / 8) * OLED_HAL_WIDTH] &= ~(1 << (y % 8));
    }
}

void bsp_oled_hal_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, oled_hal_color_t color)
{
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int16_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        bsp_oled_hal_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void bsp_oled_hal_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_hal_color_t color)
{
    uint8_t i;
    for (i = 0; i < w; i++) {
        bsp_oled_hal_draw_pixel(x + i, y, color);
        bsp_oled_hal_draw_pixel(x + i, y + h - 1, color);
    }
    for (i = 0; i < h; i++) {
        bsp_oled_hal_draw_pixel(x, y + i, color);
        bsp_oled_hal_draw_pixel(x + w - 1, y + i, color);
    }
}

void bsp_oled_hal_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_hal_color_t color)
{
    uint8_t i, j;
    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            bsp_oled_hal_draw_pixel(x + i, y + j, color);
        }
    }
}

void bsp_oled_hal_draw_string(uint8_t x, uint8_t y, const char *str, oled_hal_color_t color)
{
    while (*str) {
        if (*str >= ' ' && *str <= 'Z') {
            uint8_t c = *str - ' ';
            uint8_t i, j;
            for (i = 0; i < 6; i++) {
                uint8_t line = font6x8[c * 6 + i];
                for (j = 0; j < 8; j++) {
                    if (line & (1 << j)) {
                        bsp_oled_hal_draw_pixel(x + i, y + j, color);
                    }
                }
            }
        }
        x += 6;
        str++;
    }
}

void bsp_oled_hal_printf(uint8_t x, uint8_t y, oled_hal_color_t color, const char *fmt, ...)
{
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    bsp_oled_hal_draw_string(x, y, buf, color);
}

void bsp_oled_hal_draw_num(uint8_t x, uint8_t y, int32_t num, oled_hal_color_t color)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)num);
    bsp_oled_hal_draw_string(x, y, buf, color);
}

void bsp_oled_hal_display_on(void)
{
    oled_write_cmd(0x8D);
    oled_write_cmd(0x14);
    oled_write_cmd(0xAF);
}

void bsp_oled_hal_display_off(void)
{
    oled_write_cmd(0x8D);
    oled_write_cmd(0x10);
    oled_write_cmd(0xAE);
}

void bsp_oled_hal_set_contrast(uint8_t contrast)
{
    oled_write_cmd(0x81);
    oled_write_cmd(contrast);
}

#endif /* USE_HAL_DRIVER */
