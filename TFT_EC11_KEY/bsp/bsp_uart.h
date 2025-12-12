/**
 * @file bsp_uart.h
 * @brief UART串口驱动模块 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 硬件平台: STM32F407VGT6
 * @note 功能特性:
 *       - 支持USART1/2/3/6
 *       - 支持中断收发
 *       - 支持DMA收发
 *       - 环形缓冲区
 *       - printf重定向
 *       - 支持蓝牙模块通信
 *
 * @note 默认引脚配置:
 *       USART1: PA9(TX), PA10(RX)  - 调试串口
 *       USART2: PA2(TX), PA3(RX)   - 蓝牙
 *       USART3: PB10(TX), PB11(RX)
 */

#ifndef __BSP_UART_H
#define __BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* 启用的串口 */
#define BSP_UART1_ENABLE        1
#define BSP_UART2_ENABLE        1   /* 蓝牙用 */
#define BSP_UART3_ENABLE        0

/* 缓冲区大小 */
#define BSP_UART_TX_BUF_SIZE    256
#define BSP_UART_RX_BUF_SIZE    256

/* 默认波特率 */
#define BSP_UART_DEFAULT_BAUD   115200

/* 启用DMA */
#define BSP_UART_USE_DMA        1

/* 启用printf重定向 */
#define BSP_UART_PRINTF_ENABLE  1
#define BSP_UART_PRINTF_PORT    UART_PORT_1

/*=============================================================================
 *                              USART1配置 (调试串口)
 *============================================================================*/
#if BSP_UART1_ENABLE

#define UART1_TX_PORT           GPIOA
#define UART1_TX_PIN            GPIO_Pin_9
#define UART1_TX_PINSOURCE      GPIO_PinSource9
#define UART1_RX_PORT           GPIOA
#define UART1_RX_PIN            GPIO_Pin_10
#define UART1_RX_PINSOURCE      GPIO_PinSource10
#define UART1_GPIO_CLK          RCC_AHB1Periph_GPIOA
#define UART1_AF                GPIO_AF_USART1

/* DMA配置 */
#define UART1_TX_DMA_STREAM     DMA2_Stream7
#define UART1_TX_DMA_CHANNEL    DMA_Channel_4
#define UART1_RX_DMA_STREAM     DMA2_Stream2
#define UART1_RX_DMA_CHANNEL    DMA_Channel_4

#endif

/*=============================================================================
 *                              USART2配置 (蓝牙)
 *============================================================================*/
#if BSP_UART2_ENABLE

#define UART2_TX_PORT           GPIOA
#define UART2_TX_PIN            GPIO_Pin_2
#define UART2_TX_PINSOURCE      GPIO_PinSource2
#define UART2_RX_PORT           GPIOA
#define UART2_RX_PIN            GPIO_Pin_3
#define UART2_RX_PINSOURCE      GPIO_PinSource3
#define UART2_GPIO_CLK          RCC_AHB1Periph_GPIOA
#define UART2_AF                GPIO_AF_USART2

/* DMA配置 */
#define UART2_TX_DMA_STREAM     DMA1_Stream6
#define UART2_TX_DMA_CHANNEL    DMA_Channel_4
#define UART2_RX_DMA_STREAM     DMA1_Stream5
#define UART2_RX_DMA_CHANNEL    DMA_Channel_4

#endif

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief 串口端口枚举
 */
typedef enum {
    UART_PORT_1 = 0,
    UART_PORT_2,
    UART_PORT_3,
    UART_PORT_COUNT
} uart_port_t;

/**
 * @brief 串口配置结构体
 */
typedef struct {
    uint32_t baudrate;                      /**< 波特率 */
    uint16_t word_length;                   /**< 数据位 */
    uint16_t stop_bits;                     /**< 停止位 */
    uint16_t parity;                        /**< 校验位 */
    uint8_t use_dma;                        /**< 使用DMA */
} uart_config_t;

/**
 * @brief 接收回调函数类型
 * @param port 串口端口
 * @param data 接收到的数据
 * @param len 数据长度
 */
typedef void (*uart_rx_callback_t)(uart_port_t port, uint8_t *data, uint16_t len);

/**
 * @brief 发送完成回调函数类型
 * @param port 串口端口
 */
typedef void (*uart_tx_callback_t)(uart_port_t port);

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief 串口初始化
 * @param port 串口端口
 * @param config 配置参数 (NULL使用默认配置)
 * @retval 0:成功 -1:失败
 */
int bsp_uart_init(uart_port_t port, const uart_config_t *config);

/**
 * @brief 串口反初始化
 * @param port 串口端口
 */
void bsp_uart_deinit(uart_port_t port);

/**
 * @brief 发送单字节
 * @param port 串口端口
 * @param data 数据
 */
void bsp_uart_send_byte(uart_port_t port, uint8_t data);

/**
 * @brief 发送数据
 * @param port 串口端口
 * @param data 数据缓冲区
 * @param len 数据长度
 * @retval 实际发送长度
 */
uint16_t bsp_uart_send(uart_port_t port, const uint8_t *data, uint16_t len);

/**
 * @brief 发送字符串
 * @param port 串口端口
 * @param str 字符串
 */
void bsp_uart_send_string(uart_port_t port, const char *str);

/**
 * @brief 格式化发送
 * @param port 串口端口
 * @param fmt 格式字符串
 */
void bsp_uart_printf(uart_port_t port, const char *fmt, ...);

/**
 * @brief DMA发送
 * @param port 串口端口
 * @param data 数据缓冲区
 * @param len 数据长度
 * @retval 0:成功 -1:失败
 */
int bsp_uart_send_dma(uart_port_t port, const uint8_t *data, uint16_t len);

/**
 * @brief 接收单字节
 * @param port 串口端口
 * @param data 数据指针
 * @retval 0:成功 -1:无数据
 */
int bsp_uart_receive_byte(uart_port_t port, uint8_t *data);

/**
 * @brief 接收数据
 * @param port 串口端口
 * @param data 数据缓冲区
 * @param max_len 最大接收长度
 * @retval 实际接收长度
 */
uint16_t bsp_uart_receive(uart_port_t port, uint8_t *data, uint16_t max_len);

/**
 * @brief 获取接收缓冲区数据量
 * @param port 串口端口
 * @retval 数据量
 */
uint16_t bsp_uart_get_rx_count(uart_port_t port);

/**
 * @brief 清空接收缓冲区
 * @param port 串口端口
 */
void bsp_uart_flush_rx(uart_port_t port);

/**
 * @brief 清空发送缓冲区
 * @param port 串口端口
 */
void bsp_uart_flush_tx(uart_port_t port);

/**
 * @brief 设置接收回调
 * @param port 串口端口
 * @param callback 回调函数
 */
void bsp_uart_set_rx_callback(uart_port_t port, uart_rx_callback_t callback);

/**
 * @brief 设置发送完成回调
 * @param port 串口端口
 * @param callback 回调函数
 */
void bsp_uart_set_tx_callback(uart_port_t port, uart_tx_callback_t callback);

/**
 * @brief 设置波特率
 * @param port 串口端口
 * @param baudrate 波特率
 * @retval 0:成功 -1:失败
 */
int bsp_uart_set_baudrate(uart_port_t port, uint32_t baudrate);

/**
 * @brief 检查发送是否完成
 * @param port 串口端口
 * @retval 1:完成 0:未完成
 */
uint8_t bsp_uart_tx_complete(uart_port_t port);

/**
 * @brief 获取默认配置
 * @retval 默认配置结构体
 */
uart_config_t bsp_uart_get_default_config(void);

/*=============================================================================
 *                              便捷宏定义
 *============================================================================*/

/* 调试串口快捷函数 */
#define DEBUG_PRINT(fmt, ...)   bsp_uart_printf(UART_PORT_1, fmt, ##__VA_ARGS__)

/* 蓝牙串口快捷函数 */
#define BT_SEND(data, len)      bsp_uart_send(UART_PORT_2, data, len)
#define BT_SEND_STR(str)        bsp_uart_send_string(UART_PORT_2, str)

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H */
