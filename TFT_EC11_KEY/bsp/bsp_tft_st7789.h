/**
 * @file bsp_tft_st7789.h
 * @brief TFT显示驱动 - ST7789控制器
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 硬件平台: STM32F407VGT6
 * @note 支持分辨率: 240x240, 240x320, 135x240
 * @note 通信接口: SPI (支持硬件SPI和软件SPI)
 *
 * @note 默认引脚配置 (可通过宏修改):
 *       TFT_SCL  -> PB3  (SPI1_SCK)
 *       TFT_SDA  -> PB5  (SPI1_MOSI)
 *       TFT_RES  -> PB6  (复位)
 *       TFT_DC   -> PB7  (数据/命令选择)
 *       TFT_CS   -> PB4  (片选)
 *       TFT_BLK  -> PB8  (背光)
 */

#ifndef __BSP_TFT_ST7789_H
#define __BSP_TFT_ST7789_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/*=============================================================================
 *                              屏幕配置
 *============================================================================*/

/* 屏幕分辨率选择 - 2.4寸TFT默认240x320 */
#define TFT_240X240     0
#define TFT_240X320     1
#define TFT_135X240     0

#if TFT_240X240
    #define TFT_WIDTH   240
    #define TFT_HEIGHT  240
#elif TFT_240X320
    #define TFT_WIDTH   240
    #define TFT_HEIGHT  320
#elif TFT_135X240
    #define TFT_WIDTH   135
    #define TFT_HEIGHT  240
#endif

/* 屏幕方向 */
#define TFT_ROTATION_0      0
#define TFT_ROTATION_90     1
#define TFT_ROTATION_180    2
#define TFT_ROTATION_270    3

/*=============================================================================
 *                              硬件引脚配置
 *============================================================================*/

/* 使用硬件SPI */
#define TFT_USE_HW_SPI      1

/* SPI配置 */
#define TFT_SPI             SPI1
#define TFT_SPI_CLK         RCC_APB2Periph_SPI1

/* GPIO配置 */
#define TFT_GPIO_PORT       GPIOB
#define TFT_GPIO_CLK        RCC_AHB1Periph_GPIOB

#define TFT_SCL_PIN         GPIO_Pin_3
#define TFT_SCL_PINSOURCE   GPIO_PinSource3
#define TFT_SDA_PIN         GPIO_Pin_5
#define TFT_SDA_PINSOURCE   GPIO_PinSource5

#define TFT_CS_PIN          GPIO_Pin_4
#define TFT_DC_PIN          GPIO_Pin_7
#define TFT_RES_PIN         GPIO_Pin_6
#define TFT_BLK_PIN         GPIO_Pin_8

/* 控制宏 */
#define TFT_CS_LOW()        GPIO_ResetBits(TFT_GPIO_PORT, TFT_CS_PIN)
#define TFT_CS_HIGH()       GPIO_SetBits(TFT_GPIO_PORT, TFT_CS_PIN)
#define TFT_DC_LOW()        GPIO_ResetBits(TFT_GPIO_PORT, TFT_DC_PIN)
#define TFT_DC_HIGH()       GPIO_SetBits(TFT_GPIO_PORT, TFT_DC_PIN)
#define TFT_RES_LOW()       GPIO_ResetBits(TFT_GPIO_PORT, TFT_RES_PIN)
#define TFT_RES_HIGH()      GPIO_SetBits(TFT_GPIO_PORT, TFT_RES_PIN)
#define TFT_BLK_ON()        GPIO_SetBits(TFT_GPIO_PORT, TFT_BLK_PIN)
#define TFT_BLK_OFF()       GPIO_ResetBits(TFT_GPIO_PORT, TFT_BLK_PIN)

/*=============================================================================
 *                              颜色定义 (RGB565)
 *============================================================================*/

#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_ORANGE      0xFD20
#define TFT_PINK        0xFC18
#define TFT_PURPLE      0x8010
#define TFT_GRAY        0x8410
#define TFT_DARKGRAY    0x4208
#define TFT_LIGHTGRAY   0xC618
#define TFT_BROWN       0xA145
#define TFT_NAVY        0x0010
#define TFT_DARKGREEN   0x03E0
#define TFT_OLIVE       0x7BE0
#define TFT_MAROON      0x7800
#define TFT_TEAL        0x0410

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief 颜色类型
 */
typedef uint16_t tft_color_t;

/**
 * @brief 字体结构体
 */
typedef struct {
    const uint8_t *data;        /**< 字模数据 */
    uint8_t width;              /**< 字符宽度 */
    uint8_t height;             /**< 字符高度 */
    uint8_t first_char;         /**< 起始字符 */
    uint8_t last_char;          /**< 结束字符 */
} tft_font_t;

/**
 * @brief 图像结构体
 */
typedef struct {
    const uint16_t *data;       /**< 图像数据 (RGB565) */
    uint16_t width;             /**< 图像宽度 */
    uint16_t height;            /**< 图像高度 */
} tft_image_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/*----------------------- 初始化函数 -----------------------*/

/**
 * @brief TFT初始化
 * @retval 0:成功 -1:失败
 */
int bsp_tft_init(void);

/**
 * @brief TFT反初始化
 */
void bsp_tft_deinit(void);

/**
 * @brief 设置屏幕方向
 * @param rotation 方向 (0/90/180/270度)
 */
void bsp_tft_set_rotation(uint8_t rotation);

/**
 * @brief 设置背光亮度
 * @param brightness 亮度 (0-100)
 */
void bsp_tft_set_brightness(uint8_t brightness);

/**
 * @brief 开启背光
 */
void bsp_tft_backlight_on(void);

/**
 * @brief 关闭背光
 */
void bsp_tft_backlight_off(void);

/*----------------------- 基本绘图函数 -----------------------*/

/**
 * @brief 清屏
 * @param color 填充颜色
 */
void bsp_tft_clear(tft_color_t color);

/**
 * @brief 设置显示窗口
 * @param x0 起始X坐标
 * @param y0 起始Y坐标
 * @param x1 结束X坐标
 * @param y1 结束Y坐标
 */
void bsp_tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief 画点
 * @param x X坐标
 * @param y Y坐标
 * @param color 颜色
 */
void bsp_tft_draw_pixel(uint16_t x, uint16_t y, tft_color_t color);

/**
 * @brief 读取像素颜色
 * @param x X坐标
 * @param y Y坐标
 * @retval 像素颜色
 */
tft_color_t bsp_tft_read_pixel(uint16_t x, uint16_t y);

/**
 * @brief 画水平线
 * @param x 起始X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param color 颜色
 */
void bsp_tft_draw_hline(uint16_t x, uint16_t y, uint16_t w, tft_color_t color);

/**
 * @brief 画垂直线
 * @param x X坐标
 * @param y 起始Y坐标
 * @param h 高度
 * @param color 颜色
 */
void bsp_tft_draw_vline(uint16_t x, uint16_t y, uint16_t h, tft_color_t color);

/**
 * @brief 画线 (Bresenham算法)
 * @param x0 起点X
 * @param y0 起点Y
 * @param x1 终点X
 * @param y1 终点Y
 * @param color 颜色
 */
void bsp_tft_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, tft_color_t color);

/**
 * @brief 画矩形
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 */
void bsp_tft_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color);

/**
 * @brief 填充矩形
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 */
void bsp_tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color);

/**
 * @brief 画圆
 * @param x0 圆心X
 * @param y0 圆心Y
 * @param r 半径
 * @param color 颜色
 */
void bsp_tft_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, tft_color_t color);

/**
 * @brief 填充圆
 * @param x0 圆心X
 * @param y0 圆心Y
 * @param r 半径
 * @param color 颜色
 */
void bsp_tft_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, tft_color_t color);

/**
 * @brief 画圆角矩形
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param r 圆角半径
 * @param color 颜色
 */
void bsp_tft_draw_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, tft_color_t color);

/**
 * @brief 填充圆角矩形
 */
void bsp_tft_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, tft_color_t color);

/**
 * @brief 画三角形
 */
void bsp_tft_draw_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, tft_color_t color);

/**
 * @brief 填充三角形
 */
void bsp_tft_fill_triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, tft_color_t color);

/*----------------------- 文字显示函数 -----------------------*/

/**
 * @brief 显示单个字符
 * @param x X坐标
 * @param y Y坐标
 * @param ch 字符
 * @param font 字体
 * @param fg_color 前景色
 * @param bg_color 背景色
 */
void bsp_tft_draw_char(uint16_t x, uint16_t y, char ch, const tft_font_t *font, tft_color_t fg_color, tft_color_t bg_color);

/**
 * @brief 显示字符串
 * @param x X坐标
 * @param y Y坐标
 * @param str 字符串
 * @param font 字体
 * @param fg_color 前景色
 * @param bg_color 背景色
 */
void bsp_tft_draw_string(uint16_t x, uint16_t y, const char *str, const tft_font_t *font, tft_color_t fg_color, tft_color_t bg_color);

/**
 * @brief 显示格式化字符串
 */
void bsp_tft_printf(uint16_t x, uint16_t y, const tft_font_t *font, tft_color_t fg_color, tft_color_t bg_color, const char *fmt, ...);

/**
 * @brief 显示数字
 * @param x X坐标
 * @param y Y坐标
 * @param num 数字
 * @param len 显示位数
 * @param font 字体
 * @param fg_color 前景色
 * @param bg_color 背景色
 */
void bsp_tft_draw_number(uint16_t x, uint16_t y, int32_t num, uint8_t len, const tft_font_t *font, tft_color_t fg_color, tft_color_t bg_color);

/**
 * @brief 显示浮点数
 */
void bsp_tft_draw_float(uint16_t x, uint16_t y, float num, uint8_t int_len, uint8_t dec_len, const tft_font_t *font, tft_color_t fg_color, tft_color_t bg_color);

/*----------------------- 图像显示函数 -----------------------*/

/**
 * @brief 显示图像
 * @param x X坐标
 * @param y Y坐标
 * @param img 图像结构体
 */
void bsp_tft_draw_image(uint16_t x, uint16_t y, const tft_image_t *img);

/**
 * @brief 显示图像数据
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param data 图像数据
 */
void bsp_tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

/**
 * @brief 显示单色位图
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param data 位图数据
 * @param fg_color 前景色
 * @param bg_color 背景色
 */
void bsp_tft_draw_mono_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data, tft_color_t fg_color, tft_color_t bg_color);

/*----------------------- 高级功能 -----------------------*/

/**
 * @brief 滚动显示
 * @param scroll_area_top 滚动区域顶部
 * @param scroll_area_height 滚动区域高度
 * @param scroll_offset 滚动偏移
 */
void bsp_tft_scroll(uint16_t scroll_area_top, uint16_t scroll_area_height, uint16_t scroll_offset);

/**
 * @brief 反色显示
 * @param invert 1:反色 0:正常
 */
void bsp_tft_invert_display(uint8_t invert);

/**
 * @brief 设置显示开关
 * @param on 1:开 0:关
 */
void bsp_tft_display_on_off(uint8_t on);

/**
 * @brief 进入睡眠模式
 */
void bsp_tft_sleep_enter(void);

/**
 * @brief 退出睡眠模式
 */
void bsp_tft_sleep_exit(void);

/*----------------------- 颜色转换函数 -----------------------*/

/**
 * @brief RGB888转RGB565
 * @param r 红色 (0-255)
 * @param g 绿色 (0-255)
 * @param b 蓝色 (0-255)
 * @retval RGB565颜色值
 */
tft_color_t bsp_tft_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief HSV转RGB565
 * @param h 色相 (0-359)
 * @param s 饱和度 (0-100)
 * @param v 明度 (0-100)
 * @retval RGB565颜色值
 */
tft_color_t bsp_tft_hsv_to_rgb565(uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief 颜色混合
 * @param color1 颜色1
 * @param color2 颜色2
 * @param alpha 混合比例 (0-255, 0=全color2, 255=全color1)
 * @retval 混合后的颜色
 */
tft_color_t bsp_tft_color_blend(tft_color_t color1, tft_color_t color2, uint8_t alpha);

/*----------------------- 底层函数 -----------------------*/

/**
 * @brief 发送命令
 * @param cmd 命令
 */
void bsp_tft_write_cmd(uint8_t cmd);

/**
 * @brief 发送数据
 * @param data 数据
 */
void bsp_tft_write_data(uint8_t data);

/**
 * @brief 发送16位数据
 * @param data 16位数据
 */
void bsp_tft_write_data16(uint16_t data);

/**
 * @brief 批量发送颜色数据
 * @param color 颜色
 * @param count 像素数量
 */
void bsp_tft_write_color(tft_color_t color, uint32_t count);

/**
 * @brief 获取屏幕宽度
 */
uint16_t bsp_tft_get_width(void);

/**
 * @brief 获取屏幕高度
 */
uint16_t bsp_tft_get_height(void);

/*=============================================================================
 *                              内置字体声明
 *============================================================================*/

extern const tft_font_t font_8x16;
extern const tft_font_t font_12x24;
extern const tft_font_t font_16x32;

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TFT_ST7789_H */
