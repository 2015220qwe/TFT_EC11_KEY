/**
 * @file bsp_uart_hal.c
 * @brief UART串口驱动 - HAL库版本实现
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#ifdef USE_HAL_DRIVER

#include "bsp_hal/bsp_uart_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

/* 环形缓冲区 */
typedef struct {
    uint8_t buffer[UART_HAL_RX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} uart_rx_buffer_t;

/* 最多支持4个UART */
#define UART_MAX_INSTANCES  4

static uart_rx_buffer_t rx_buffers[UART_MAX_INSTANCES];
static UART_HandleTypeDef *uart_handles[UART_MAX_INSTANCES] = {NULL};
static uart_hal_rx_callback_t rx_callbacks[UART_MAX_INSTANCES] = {NULL};
static uint8_t rx_byte[UART_MAX_INSTANCES];

/*=============================================================================
 *                              私有函数
 *============================================================================*/

static int get_uart_index(UART_HandleTypeDef *huart)
{
    int i;
    for (i = 0; i < UART_MAX_INSTANCES; i++) {
        if (uart_handles[i] == huart) {
            return i;
        }
    }
    return -1;
}

static int register_uart(UART_HandleTypeDef *huart)
{
    int i;
    for (i = 0; i < UART_MAX_INSTANCES; i++) {
        if (uart_handles[i] == NULL) {
            uart_handles[i] = huart;
            return i;
        }
        if (uart_handles[i] == huart) {
            return i;
        }
    }
    return -1;
}

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief UART初始化
 */
int bsp_uart_hal_init(UART_HandleTypeDef *huart)
{
    int idx;

    if (huart == NULL) {
        return -1;
    }

    idx = register_uart(huart);
    if (idx < 0) {
        return -1;
    }

    /* 初始化缓冲区 */
    rx_buffers[idx].head = 0;
    rx_buffers[idx].tail = 0;

    return 0;
}

/**
 * @brief 发送数据
 */
uint16_t bsp_uart_hal_send(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len)
{
    if (huart == NULL || data == NULL || len == 0) {
        return 0;
    }

    if (HAL_UART_Transmit(huart, (uint8_t *)data, len, 1000) != HAL_OK) {
        return 0;
    }

    return len;
}

/**
 * @brief 发送字符串
 */
void bsp_uart_hal_send_string(UART_HandleTypeDef *huart, const char *str)
{
    if (huart == NULL || str == NULL) {
        return;
    }

    bsp_uart_hal_send(huart, (const uint8_t *)str, strlen(str));
}

/**
 * @brief 格式化发送
 */
void bsp_uart_hal_printf(UART_HandleTypeDef *huart, const char *fmt, ...)
{
    char buf[256];
    va_list args;

    if (huart == NULL || fmt == NULL) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    bsp_uart_hal_send_string(huart, buf);
}

/**
 * @brief DMA发送
 */
int bsp_uart_hal_send_dma(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len)
{
    if (huart == NULL || data == NULL || len == 0) {
        return -1;
    }

    if (HAL_UART_Transmit_DMA(huart, (uint8_t *)data, len) != HAL_OK) {
        return -1;
    }

    return 0;
}

/**
 * @brief 接收数据
 */
uint16_t bsp_uart_hal_receive(UART_HandleTypeDef *huart, uint8_t *data, uint16_t max_len)
{
    int idx;
    uint16_t count = 0;
    uart_rx_buffer_t *buf;

    idx = get_uart_index(huart);
    if (idx < 0 || data == NULL) {
        return 0;
    }

    buf = &rx_buffers[idx];

    while (buf->tail != buf->head && count < max_len) {
        data[count++] = buf->buffer[buf->tail];
        buf->tail = (buf->tail + 1) % UART_HAL_RX_BUF_SIZE;
    }

    return count;
}

/**
 * @brief 获取接收数据量
 */
uint16_t bsp_uart_hal_get_rx_count(UART_HandleTypeDef *huart)
{
    int idx;
    uart_rx_buffer_t *buf;

    idx = get_uart_index(huart);
    if (idx < 0) {
        return 0;
    }

    buf = &rx_buffers[idx];
    return (buf->head - buf->tail + UART_HAL_RX_BUF_SIZE) % UART_HAL_RX_BUF_SIZE;
}

/**
 * @brief 设置接收回调
 */
void bsp_uart_hal_set_rx_callback(UART_HandleTypeDef *huart, uart_hal_rx_callback_t callback)
{
    int idx;

    idx = get_uart_index(huart);
    if (idx < 0) {
        idx = register_uart(huart);
    }

    if (idx >= 0) {
        rx_callbacks[idx] = callback;
    }
}

/**
 * @brief 启动接收中断
 */
int bsp_uart_hal_start_receive_it(UART_HandleTypeDef *huart)
{
    int idx;

    idx = get_uart_index(huart);
    if (idx < 0) {
        idx = register_uart(huart);
    }

    if (idx < 0) {
        return -1;
    }

    if (HAL_UART_Receive_IT(huart, &rx_byte[idx], 1) != HAL_OK) {
        return -1;
    }

    return 0;
}

/**
 * @brief 接收完成回调 (在HAL回调中调用)
 */
void bsp_uart_hal_rx_complete_callback(UART_HandleTypeDef *huart)
{
    int idx;
    uart_rx_buffer_t *buf;
    uint16_t next;

    idx = get_uart_index(huart);
    if (idx < 0) {
        return;
    }

    buf = &rx_buffers[idx];

    /* 存入缓冲区 */
    next = (buf->head + 1) % UART_HAL_RX_BUF_SIZE;
    if (next != buf->tail) {
        buf->buffer[buf->head] = rx_byte[idx];
        buf->head = next;
    }

    /* 回调 */
    if (rx_callbacks[idx] != NULL) {
        rx_callbacks[idx](&rx_byte[idx], 1);
    }

    /* 继续接收 */
    HAL_UART_Receive_IT(huart, &rx_byte[idx], 1);
}

#endif /* USE_HAL_DRIVER */
