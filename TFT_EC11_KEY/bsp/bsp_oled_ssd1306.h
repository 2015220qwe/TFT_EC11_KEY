/**
 * @file bsp_oled_ssd1306.h
 * @brief SSD1306 OLED显示驱动 - 支持0.96寸(128x64)和0.91寸(128x32)
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 功能特性:
 *       - 支持I2C/SPI接口
 *       - 支持软件/硬件I2C
 *       - 支持0.96寸 128x64分辨率
 *       - 支持0.91寸 128x32分辨率
 *       - 内置显存缓冲区
 *       - 图形绘制函数
 *       - 字符/字符串显示
 */

#ifndef __BSP_OLED_SSD1306_H
#define __BSP_OLED_SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*=============================================================================
 *                              配置选项
 *============================================================================*/

/* OLED屏幕类型选择 */
#define OLED_TYPE_096_128X64    0   /* 0.96寸 128x64 */
#define OLED_TYPE_091_128X32    1   /* 0.91寸 128x32 */

/* 当前使用的屏幕类型 */
#ifndef OLED_TYPE
#define OLED_TYPE               OLED_TYPE_096_128X64
#endif

/* 通信接口选择 */
#define OLED_USE_HW_I2C         1   /* 硬件I2C */
#define OLED_USE_SW_I2C         0   /* 软件I2C */
#define OLED_USE_SPI            0   /* SPI接口 */

/* 屏幕尺寸 */
#define OLED_WIDTH              128
#if OLED_TYPE == OLED_TYPE_096_128X64
#define OLED_HEIGHT             64
#define OLED_PAGES              8
#else
#define OLED_HEIGHT             32
#define OLED_PAGES              4
#endif

/* I2C地址 (7位地址) */
#define OLED_I2C_ADDR           0x3C    /* 或0x3D */
#define OLED_I2C_ADDR_8BIT      (OLED_I2C_ADDR << 1)

/*=============================================================================
 *                              引脚配置 (标准库)
 *============================================================================*/

/* 硬件I2C引脚 */
#define OLED_I2C_PORT           I2C1
#define OLED_I2C_SCL_PORT       GPIOB
#define OLED_I2C_SCL_PIN        GPIO_Pin_6
#define OLED_I2C_SDA_PORT       GPIOB
#define OLED_I2C_SDA_PIN        GPIO_Pin_7
#define OLED_I2C_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define OLED_I2C_PERIPH_CLK     RCC_APB1Periph_I2C1
#define OLED_I2C_SPEED          400000  /* 400kHz */

/* 软件I2C引脚 */
#define OLED_SW_SCL_PORT        GPIOB
#define OLED_SW_SCL_PIN         GPIO_Pin_6
#define OLED_SW_SDA_PORT        GPIOB
#define OLED_SW_SDA_PIN         GPIO_Pin_7

/* SPI引脚 */
#define OLED_SPI_PORT           SPI1
#define OLED_SPI_SCK_PORT       GPIOA
#define OLED_SPI_SCK_PIN        GPIO_Pin_5
#define OLED_SPI_MOSI_PORT      GPIOA
#define OLED_SPI_MOSI_PIN       GPIO_Pin_7
#define OLED_SPI_CS_PORT        GPIOA
#define OLED_SPI_CS_PIN         GPIO_Pin_4
#define OLED_SPI_DC_PORT        GPIOA
#define OLED_SPI_DC_PIN         GPIO_Pin_3
#define OLED_SPI_RST_PORT       GPIOA
#define OLED_SPI_RST_PIN        GPIO_Pin_2

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief OLED颜色 (单色显示)
 */
typedef enum {
    OLED_COLOR_BLACK = 0,   /**< 黑色/不亮 */
    OLED_COLOR_WHITE = 1    /**< 白色/亮 */
} oled_color_t;

/**
 * @brief 字体定义
 */
typedef struct {
    uint8_t width;          /**< 字符宽度 */
    uint8_t height;         /**< 字符高度 */
    const uint8_t *data;    /**< 字模数据 */
} oled_font_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/*----------------------- 初始化函数 -----------------------*/

/**
 * @brief OLED初始化
 * @retval 0:成功 -1:失败
 */
int bsp_oled_init(void);

/**
 * @brief OLED反初始化
 */
void bsp_oled_deinit(void);

/**
 * @brief 设置OLED类型 (运行时切换)
 * @param type OLED_TYPE_096_128X64 或 OLED_TYPE_091_128X32
 */
void bsp_oled_set_type(uint8_t type);

/**
 * @brief 获取当前OLED类型
 * @retval OLED类型
 */
uint8_t bsp_oled_get_type(void);

/*----------------------- 显示控制 -----------------------*/

/**
 * @brief 开启显示
 */
void bsp_oled_display_on(void);

/**
 * @brief 关闭显示
 */
void bsp_oled_display_off(void);

/**
 * @brief 设置对比度
 * @param contrast 对比度 (0-255)
 */
void bsp_oled_set_contrast(uint8_t contrast);

/**
 * @brief 反转显示
 * @param invert 1:反转 0:正常
 */
void bsp_oled_invert_display(uint8_t invert);

/**
 * @brief 刷新显示 (将缓冲区内容写入OLED)
 */
void bsp_oled_refresh(void);

/**
 * @brief 清屏
 */
void bsp_oled_clear(void);

/**
 * @brief 填充屏幕
 * @param color 颜色
 */
void bsp_oled_fill(oled_color_t color);

/*----------------------- 基本绘图 -----------------------*/

/**
 * @brief 画点
 * @param x X坐标
 * @param y Y坐标
 * @param color 颜色
 */
void bsp_oled_draw_pixel(uint8_t x, uint8_t y, oled_color_t color);

/**
 * @brief 获取点颜色
 * @param x X坐标
 * @param y Y坐标
 * @retval 颜色
 */
oled_color_t bsp_oled_get_pixel(uint8_t x, uint8_t y);

/**
 * @brief 画水平线
 * @param x 起始X
 * @param y 起始Y
 * @param w 宽度
 * @param color 颜色
 */
void bsp_oled_draw_hline(uint8_t x, uint8_t y, uint8_t w, oled_color_t color);

/**
 * @brief 画垂直线
 * @param x 起始X
 * @param y 起始Y
 * @param h 高度
 * @param color 颜色
 */
void bsp_oled_draw_vline(uint8_t x, uint8_t y, uint8_t h, oled_color_t color);

/**
 * @brief 画线
 * @param x0 起点X
 * @param y0 起点Y
 * @param x1 终点X
 * @param y1 终点Y
 * @param color 颜色
 */
void bsp_oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, oled_color_t color);

/**
 * @brief 画矩形
 * @param x 左上角X
 * @param y 左上角Y
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 */
void bsp_oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color);

/**
 * @brief 填充矩形
 * @param x 左上角X
 * @param y 左上角Y
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 */
void bsp_oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color);

/**
 * @brief 画圆
 * @param x0 圆心X
 * @param y0 圆心Y
 * @param r 半径
 * @param color 颜色
 */
void bsp_oled_draw_circle(uint8_t x0, uint8_t y0, uint8_t r, oled_color_t color);

/**
 * @brief 填充圆
 * @param x0 圆心X
 * @param y0 圆心Y
 * @param r 半径
 * @param color 颜色
 */
void bsp_oled_fill_circle(uint8_t x0, uint8_t y0, uint8_t r, oled_color_t color);

/*----------------------- 文字显示 -----------------------*/

/**
 * @brief 显示字符
 * @param x X坐标
 * @param y Y坐标
 * @param ch 字符
 * @param font 字体
 * @param color 颜色
 */
void bsp_oled_draw_char(uint8_t x, uint8_t y, char ch, const oled_font_t *font, oled_color_t color);

/**
 * @brief 显示字符串
 * @param x X坐标
 * @param y Y坐标
 * @param str 字符串
 * @param font 字体
 * @param color 颜色
 */
void bsp_oled_draw_string(uint8_t x, uint8_t y, const char *str, const oled_font_t *font, oled_color_t color);

/**
 * @brief 格式化显示
 * @param x X坐标
 * @param y Y坐标
 * @param font 字体
 * @param color 颜色
 * @param fmt 格式字符串
 */
void bsp_oled_printf(uint8_t x, uint8_t y, const oled_font_t *font, oled_color_t color, const char *fmt, ...);

/**
 * @brief 显示数字
 * @param x X坐标
 * @param y Y坐标
 * @param num 数字
 * @param font 字体
 * @param color 颜色
 */
void bsp_oled_draw_num(uint8_t x, uint8_t y, int32_t num, const oled_font_t *font, oled_color_t color);

/**
 * @brief 显示浮点数
 * @param x X坐标
 * @param y Y坐标
 * @param num 数字
 * @param decimals 小数位数
 * @param font 字体
 * @param color 颜色
 */
void bsp_oled_draw_float(uint8_t x, uint8_t y, float num, uint8_t decimals, const oled_font_t *font, oled_color_t color);

/*----------------------- 图像显示 -----------------------*/

/**
 * @brief 显示位图
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param bitmap 位图数据
 */
void bsp_oled_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap);

/**
 * @brief 显示XBM图像
 * @param x X坐标
 * @param y Y坐标
 * @param w 宽度
 * @param h 高度
 * @param xbm XBM数据
 */
void bsp_oled_draw_xbm(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *xbm);

/*----------------------- 缓冲区操作 -----------------------*/

/**
 * @brief 获取显存缓冲区指针
 * @retval 缓冲区指针
 */
uint8_t *bsp_oled_get_buffer(void);

/**
 * @brief 获取缓冲区大小
 * @retval 缓冲区大小 (字节)
 */
uint16_t bsp_oled_get_buffer_size(void);

/**
 * @brief 设置显存缓冲区
 * @param buffer 缓冲区数据
 */
void bsp_oled_set_buffer(const uint8_t *buffer);

/*----------------------- 滚动效果 -----------------------*/

/**
 * @brief 开始水平滚动
 * @param direction 0:右滚动 1:左滚动
 * @param start_page 起始页 (0-7)
 * @param end_page 结束页 (0-7)
 * @param speed 速度 (0-7, 0最快)
 */
void bsp_oled_scroll_horizontal(uint8_t direction, uint8_t start_page, uint8_t end_page, uint8_t speed);

/**
 * @brief 开始垂直和水平滚动
 * @param direction 0:右+向上 1:左+向上
 * @param start_page 起始页
 * @param end_page 结束页
 * @param speed 速度
 * @param vertical_offset 垂直偏移
 */
void bsp_oled_scroll_diagonal(uint8_t direction, uint8_t start_page, uint8_t end_page, uint8_t speed, uint8_t vertical_offset);

/**
 * @brief 停止滚动
 */
void bsp_oled_scroll_stop(void);

/*=============================================================================
 *                              内置字体
 *============================================================================*/

extern const oled_font_t oled_font_6x8;      /**< 6x8 ASCII字体 */
extern const oled_font_t oled_font_8x16;     /**< 8x16 ASCII字体 */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_OLED_SSD1306_H */
