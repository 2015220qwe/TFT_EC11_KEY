/**
 * @file bsp_oled_hal.h
 * @brief SSD1306 OLED驱动 - HAL库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#ifndef __BSP_OLED_HAL_H
#define __BSP_OLED_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>

/*=============================================================================
 *                              配置
 *============================================================================*/

#define OLED_HAL_WIDTH      128
#define OLED_HAL_HEIGHT_64  64
#define OLED_HAL_HEIGHT_32  32

#define OLED_HAL_I2C_ADDR   (0x3C << 1)

/*=============================================================================
 *                              类型定义
 *============================================================================*/

typedef enum {
    OLED_HAL_TYPE_128X64 = 0,
    OLED_HAL_TYPE_128X32
} oled_hal_type_t;

typedef enum {
    OLED_HAL_BLACK = 0,
    OLED_HAL_WHITE = 1
} oled_hal_color_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

#ifdef USE_HAL_DRIVER

/**
 * @brief OLED初始化 (I2C模式)
 * @param hi2c I2C句柄
 * @param type 屏幕类型 (128x64 或 128x32)
 * @retval 0:成功 -1:失败
 */
int bsp_oled_hal_init(I2C_HandleTypeDef *hi2c, oled_hal_type_t type);

/**
 * @brief 刷新显示
 */
void bsp_oled_hal_refresh(void);

/**
 * @brief 清屏
 */
void bsp_oled_hal_clear(void);

/**
 * @brief 填充屏幕
 */
void bsp_oled_hal_fill(oled_hal_color_t color);

/**
 * @brief 画点
 */
void bsp_oled_hal_draw_pixel(uint8_t x, uint8_t y, oled_hal_color_t color);

/**
 * @brief 画线
 */
void bsp_oled_hal_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, oled_hal_color_t color);

/**
 * @brief 画矩形
 */
void bsp_oled_hal_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_hal_color_t color);

/**
 * @brief 填充矩形
 */
void bsp_oled_hal_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_hal_color_t color);

/**
 * @brief 显示字符串
 */
void bsp_oled_hal_draw_string(uint8_t x, uint8_t y, const char *str, oled_hal_color_t color);

/**
 * @brief 格式化显示
 */
void bsp_oled_hal_printf(uint8_t x, uint8_t y, oled_hal_color_t color, const char *fmt, ...);

/**
 * @brief 显示数字
 */
void bsp_oled_hal_draw_num(uint8_t x, uint8_t y, int32_t num, oled_hal_color_t color);

/**
 * @brief 开启显示
 */
void bsp_oled_hal_display_on(void);

/**
 * @brief 关闭显示
 */
void bsp_oled_hal_display_off(void);

/**
 * @brief 设置对比度
 */
void bsp_oled_hal_set_contrast(uint8_t contrast);

#endif /* USE_HAL_DRIVER */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_OLED_HAL_H */
