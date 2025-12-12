/**
 * @file bsp_uart_hal.h
 * @brief UART串口驱动 - HAL库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#ifndef __BSP_UART_HAL_H
#define __BSP_UART_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>

/*=============================================================================
 *                              宏定义
 *============================================================================*/

#define UART_HAL_RX_BUF_SIZE    256
#define UART_HAL_TX_BUF_SIZE    256

/*=============================================================================
 *                              类型定义
 *============================================================================*/

typedef void (*uart_hal_rx_callback_t)(uint8_t *data, uint16_t len);

/*=============================================================================
 *                              函数声明
 *============================================================================*/

#ifdef USE_HAL_DRIVER

/**
 * @brief UART初始化
 * @param huart UART句柄
 * @retval 0:成功 -1:失败
 */
int bsp_uart_hal_init(UART_HandleTypeDef *huart);

/**
 * @brief 发送数据
 * @param huart UART句柄
 * @param data 数据
 * @param len 长度
 * @retval 发送长度
 */
uint16_t bsp_uart_hal_send(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len);

/**
 * @brief 发送字符串
 */
void bsp_uart_hal_send_string(UART_HandleTypeDef *huart, const char *str);

/**
 * @brief 格式化发送
 */
void bsp_uart_hal_printf(UART_HandleTypeDef *huart, const char *fmt, ...);

/**
 * @brief DMA发送
 */
int bsp_uart_hal_send_dma(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len);

/**
 * @brief 接收数据
 */
uint16_t bsp_uart_hal_receive(UART_HandleTypeDef *huart, uint8_t *data, uint16_t max_len);

/**
 * @brief 获取接收数据量
 */
uint16_t bsp_uart_hal_get_rx_count(UART_HandleTypeDef *huart);

/**
 * @brief 设置接收回调
 */
void bsp_uart_hal_set_rx_callback(UART_HandleTypeDef *huart, uart_hal_rx_callback_t callback);

/**
 * @brief 启动接收中断
 */
int bsp_uart_hal_start_receive_it(UART_HandleTypeDef *huart);

/**
 * @brief 接收完成回调 (在HAL回调中调用)
 */
void bsp_uart_hal_rx_complete_callback(UART_HandleTypeDef *huart);

#endif /* USE_HAL_DRIVER */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_HAL_H */
