/**
 * @file bsp_tft_hal.h
 * @brief TFT显示驱动 - HAL库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 此文件为HAL库版本，兼容STM32CubeMX配置
 */

#ifndef __BSP_TFT_HAL_H
#define __BSP_TFT_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>

/*=============================================================================
 *                              屏幕配置
 *============================================================================*/

#define TFT_WIDTH   240
#define TFT_HEIGHT  240

/*=============================================================================
 *                              颜色定义 (RGB565)
 *============================================================================*/

typedef uint16_t tft_color_t;

#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F

/*=============================================================================
 *                              函数声明
 *============================================================================*/

#ifdef USE_HAL_DRIVER

/**
 * @brief TFT初始化 (HAL版本)
 * @param hspi SPI句柄指针
 * @param cs_port CS端口
 * @param cs_pin CS引脚
 * @param dc_port DC端口
 * @param dc_pin DC引脚
 * @param rst_port RST端口
 * @param rst_pin RST引脚
 * @retval 0:成功 -1:失败
 */
int bsp_tft_hal_init(SPI_HandleTypeDef *hspi,
                     GPIO_TypeDef *cs_port, uint16_t cs_pin,
                     GPIO_TypeDef *dc_port, uint16_t dc_pin,
                     GPIO_TypeDef *rst_port, uint16_t rst_pin);

/**
 * @brief 清屏
 * @param color 颜色
 */
void bsp_tft_hal_clear(tft_color_t color);

/**
 * @brief 画点
 * @param x X坐标
 * @param y Y坐标
 * @param color 颜色
 */
void bsp_tft_hal_draw_pixel(uint16_t x, uint16_t y, tft_color_t color);

/**
 * @brief 填充矩形
 */
void bsp_tft_hal_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color);

/**
 * @brief 画线
 */
void bsp_tft_hal_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, tft_color_t color);

/**
 * @brief 显示字符串
 */
void bsp_tft_hal_draw_string(uint16_t x, uint16_t y, const char *str, tft_color_t fg, tft_color_t bg);

/**
 * @brief 显示图像
 */
void bsp_tft_hal_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

/**
 * @brief 设置屏幕方向
 */
void bsp_tft_hal_set_rotation(uint8_t rotation);

/**
 * @brief DMA传输 (加速)
 */
int bsp_tft_hal_dma_transfer(const uint16_t *data, uint32_t len);

#endif /* USE_HAL_DRIVER */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TFT_HAL_H */
