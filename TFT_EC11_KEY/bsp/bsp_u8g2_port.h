/**
  ******************************************************************************
  * @file    bsp_u8g2_port.h
  * @author  TFT_EC11_KEY Project
  * @brief   U8G2显示库移植适配层 - STM32F407
  ******************************************************************************
  * @attention
  *
  * 支持的通信方式:
  * - 硬件I2C
  * - 软件I2C
  * - 硬件SPI
  * - 软件SPI
  *
  * 支持的显示器:
  * - SSD1306 OLED 128x64
  * - SSD1306 OLED 128x32
  * - ST7920 LCD 128x64
  * - ST7789 TFT 240x240
  * - 其他U8G2支持的显示器
  *
  ******************************************************************************
  */

#ifndef __BSP_U8G2_PORT_H
#define __BSP_U8G2_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "u8g2.h"

/* 配置选项 ------------------------------------------------------------------*/
/* 通信方式选择 */
#define U8G2_USE_HW_I2C      1    /* 使用硬件I2C */
#define U8G2_USE_SW_I2C      0    /* 使用软件I2C */
#define U8G2_USE_HW_SPI      0    /* 使用硬件SPI */
#define U8G2_USE_SW_SPI      0    /* 使用软件SPI */

/* I2C引脚定义 (硬件I2C) */
#define U8G2_I2C_PORT        I2C1
#define U8G2_I2C_SCL_PORT    GPIOB
#define U8G2_I2C_SCL_PIN     GPIO_Pin_6
#define U8G2_I2C_SDA_PORT    GPIOB
#define U8G2_I2C_SDA_PIN     GPIO_Pin_7
#define U8G2_I2C_CLK         RCC_AHB1Periph_GPIOB
#define U8G2_I2C_PERIPH_CLK  RCC_APB1Periph_I2C1

/* SPI引脚定义 (软件SPI) */
#define U8G2_SPI_SCK_PORT    GPIOA
#define U8G2_SPI_SCK_PIN     GPIO_Pin_5
#define U8G2_SPI_MOSI_PORT   GPIOA
#define U8G2_SPI_MOSI_PIN    GPIO_Pin_7
#define U8G2_SPI_CS_PORT     GPIOA
#define U8G2_SPI_CS_PIN      GPIO_Pin_4
#define U8G2_SPI_DC_PORT     GPIOA
#define U8G2_SPI_DC_PIN      GPIO_Pin_3
#define U8G2_SPI_RST_PORT    GPIOA
#define U8G2_SPI_RST_PIN     GPIO_Pin_2

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  U8G2硬件初始化
 * @param  u8g2: U8G2对象指针
 * @retval None
 */
void bsp_u8g2_hw_init(u8g2_t *u8g2);

/**
 * @brief  U8G2 GPIO和延时回调函数
 * @param  u8x8: U8X8对象指针
 * @param  msg: 消息类型
 * @param  arg_int: 整数参数
 * @param  arg_ptr: 指针参数
 * @retval 返回值
 */
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief  U8G2硬件I2C字节传输回调
 * @param  u8x8: U8X8对象指针
 * @param  msg: 消息类型
 * @param  arg_int: 整数参数
 * @param  arg_ptr: 指针参数
 * @retval 返回值
 */
uint8_t u8g2_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

/**
 * @brief  U8G2软件I2C字节传输回调
 * @param  u8x8: U8X8对象指针
 * @param  msg: 消息类型
 * @param  arg_int: 整数参数
 * @param  arg_ptr: 指针参数
 * @retval 返回值
 */
uint8_t u8g2_byte_sw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_U8G2_PORT_H */
