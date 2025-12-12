/**
 * @file bsp_uart.c
 * @brief UART串口驱动实现 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_uart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*=============================================================================
 *                              环形缓冲区定义
 *============================================================================*/

typedef struct {
    uint8_t buffer[BSP_UART_RX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} ring_buffer_t;

/*=============================================================================
 *                              私有变量
 *============================================================================*/

#if BSP_UART1_ENABLE
static ring_buffer_t uart1_rx_buf = {0};
static uint8_t uart1_tx_buf[BSP_UART_TX_BUF_SIZE];
static volatile uint8_t uart1_tx_busy = 0;
static uart_rx_callback_t uart1_rx_cb = NULL;
static uart_tx_callback_t uart1_tx_cb = NULL;
#endif

#if BSP_UART2_ENABLE
static ring_buffer_t uart2_rx_buf = {0};
static uint8_t uart2_tx_buf[BSP_UART_TX_BUF_SIZE];
static volatile uint8_t uart2_tx_busy = 0;
static uart_rx_callback_t uart2_rx_cb = NULL;
static uart_tx_callback_t uart2_tx_cb = NULL;
#endif

static const USART_TypeDef* uart_instances[] = {
    USART1, USART2, USART3
};

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static void uart1_gpio_init(void);
static void uart2_gpio_init(void);
static void ring_buffer_put(ring_buffer_t *rb, uint8_t data);
static int ring_buffer_get(ring_buffer_t *rb, uint8_t *data);
static uint16_t ring_buffer_count(ring_buffer_t *rb);
static void ring_buffer_flush(ring_buffer_t *rb);

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief 获取默认配置
 */
uart_config_t bsp_uart_get_default_config(void)
{
    uart_config_t config = {
        .baudrate = BSP_UART_DEFAULT_BAUD,
        .word_length = USART_WordLength_8b,
        .stop_bits = USART_StopBits_1,
        .parity = USART_Parity_No,
        .use_dma = 0
    };
    return config;
}

/**
 * @brief 串口初始化
 */
int bsp_uart_init(uart_port_t port, const uart_config_t *config)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uart_config_t cfg;

    if (config == NULL) {
        cfg = bsp_uart_get_default_config();
    } else {
        cfg = *config;
    }

    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1:
        /* 使能时钟 */
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

        /* GPIO初始化 */
        uart1_gpio_init();

        /* USART配置 */
        USART_InitStructure.USART_BaudRate = cfg.baudrate;
        USART_InitStructure.USART_WordLength = cfg.word_length;
        USART_InitStructure.USART_StopBits = cfg.stop_bits;
        USART_InitStructure.USART_Parity = cfg.parity;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(USART1, &USART_InitStructure);

        /* 使能接收中断 */
        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

        /* NVIC配置 */
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        /* 使能USART */
        USART_Cmd(USART1, ENABLE);

        /* 清空缓冲区 */
        ring_buffer_flush(&uart1_rx_buf);
        uart1_tx_busy = 0;
        break;
#endif

#if BSP_UART2_ENABLE
    case UART_PORT_2:
        /* 使能时钟 */
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

        /* GPIO初始化 */
        uart2_gpio_init();

        /* USART配置 */
        USART_InitStructure.USART_BaudRate = cfg.baudrate;
        USART_InitStructure.USART_WordLength = cfg.word_length;
        USART_InitStructure.USART_StopBits = cfg.stop_bits;
        USART_InitStructure.USART_Parity = cfg.parity;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(USART2, &USART_InitStructure);

        /* 使能接收中断 */
        USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

        /* NVIC配置 */
        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        /* 使能USART */
        USART_Cmd(USART2, ENABLE);

        /* 清空缓冲区 */
        ring_buffer_flush(&uart2_rx_buf);
        uart2_tx_busy = 0;
        break;
#endif

    default:
        return -1;
    }

    return 0;
}

/**
 * @brief 串口反初始化
 */
void bsp_uart_deinit(uart_port_t port)
{
    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1:
        USART_Cmd(USART1, DISABLE);
        USART_DeInit(USART1);
        break;
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2:
        USART_Cmd(USART2, DISABLE);
        USART_DeInit(USART2);
        break;
#endif
    default:
        break;
    }
}

/**
 * @brief 发送单字节
 */
void bsp_uart_send_byte(uart_port_t port, uint8_t data)
{
    USART_TypeDef *uart = NULL;

    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1: uart = USART1; break;
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2: uart = USART2; break;
#endif
    default: return;
    }

    while (USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET);
    USART_SendData(uart, data);
}

/**
 * @brief 发送数据
 */
uint16_t bsp_uart_send(uart_port_t port, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len; i++) {
        bsp_uart_send_byte(port, data[i]);
    }

    return len;
}

/**
 * @brief 发送字符串
 */
void bsp_uart_send_string(uart_port_t port, const char *str)
{
    while (*str) {
        bsp_uart_send_byte(port, *str++);
    }
}

/**
 * @brief 格式化发送
 */
void bsp_uart_printf(uart_port_t port, const char *fmt, ...)
{
    char buf[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    bsp_uart_send_string(port, buf);
}

/**
 * @brief 接收单字节
 */
int bsp_uart_receive_byte(uart_port_t port, uint8_t *data)
{
    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1:
        return ring_buffer_get(&uart1_rx_buf, data);
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2:
        return ring_buffer_get(&uart2_rx_buf, data);
#endif
    default:
        return -1;
    }
}

/**
 * @brief 接收数据
 */
uint16_t bsp_uart_receive(uart_port_t port, uint8_t *data, uint16_t max_len)
{
    uint16_t count = 0;

    while (count < max_len) {
        if (bsp_uart_receive_byte(port, &data[count]) == 0) {
            count++;
        } else {
            break;
        }
    }

    return count;
}

/**
 * @brief 获取接收缓冲区数据量
 */
uint16_t bsp_uart_get_rx_count(uart_port_t port)
{
    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1:
        return ring_buffer_count(&uart1_rx_buf);
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2:
        return ring_buffer_count(&uart2_rx_buf);
#endif
    default:
        return 0;
    }
}

/**
 * @brief 清空接收缓冲区
 */
void bsp_uart_flush_rx(uart_port_t port)
{
    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1:
        ring_buffer_flush(&uart1_rx_buf);
        break;
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2:
        ring_buffer_flush(&uart2_rx_buf);
        break;
#endif
    default:
        break;
    }
}

/**
 * @brief 设置接收回调
 */
void bsp_uart_set_rx_callback(uart_port_t port, uart_rx_callback_t callback)
{
    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1:
        uart1_rx_cb = callback;
        break;
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2:
        uart2_rx_cb = callback;
        break;
#endif
    default:
        break;
    }
}

/**
 * @brief 设置波特率
 */
int bsp_uart_set_baudrate(uart_port_t port, uint32_t baudrate)
{
    USART_TypeDef *uart = NULL;

    switch (port) {
#if BSP_UART1_ENABLE
    case UART_PORT_1: uart = USART1; break;
#endif
#if BSP_UART2_ENABLE
    case UART_PORT_2: uart = USART2; break;
#endif
    default: return -1;
    }

    USART_Cmd(uart, DISABLE);

    /* 重新配置波特率 */
    uart_config_t cfg = bsp_uart_get_default_config();
    cfg.baudrate = baudrate;
    bsp_uart_deinit(port);
    bsp_uart_init(port, &cfg);

    return 0;
}

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

#if BSP_UART1_ENABLE
static void uart1_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(UART1_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = UART1_TX_PIN | UART1_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(UART1_TX_PORT, &GPIO_InitStructure);

    GPIO_PinAFConfig(UART1_TX_PORT, UART1_TX_PINSOURCE, UART1_AF);
    GPIO_PinAFConfig(UART1_RX_PORT, UART1_RX_PINSOURCE, UART1_AF);
}
#endif

#if BSP_UART2_ENABLE
static void uart2_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(UART2_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = UART2_TX_PIN | UART2_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(UART2_TX_PORT, &GPIO_InitStructure);

    GPIO_PinAFConfig(UART2_TX_PORT, UART2_TX_PINSOURCE, UART2_AF);
    GPIO_PinAFConfig(UART2_RX_PORT, UART2_RX_PINSOURCE, UART2_AF);
}
#endif

static void ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    uint16_t next = (rb->head + 1) % BSP_UART_RX_BUF_SIZE;
    if (next != rb->tail) {
        rb->buffer[rb->head] = data;
        rb->head = next;
    }
}

static int ring_buffer_get(ring_buffer_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail) {
        return -1;
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % BSP_UART_RX_BUF_SIZE;
    return 0;
}

static uint16_t ring_buffer_count(ring_buffer_t *rb)
{
    return (rb->head - rb->tail + BSP_UART_RX_BUF_SIZE) % BSP_UART_RX_BUF_SIZE;
}

static void ring_buffer_flush(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/*=============================================================================
 *                              中断服务函数
 *============================================================================*/

#if BSP_UART1_ENABLE
void USART1_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        data = USART_ReceiveData(USART1);
        ring_buffer_put(&uart1_rx_buf, data);

        if (uart1_rx_cb != NULL) {
            uart1_rx_cb(UART_PORT_1, &data, 1);
        }
    }

    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET) {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        uart1_tx_busy = 0;
        if (uart1_tx_cb != NULL) {
            uart1_tx_cb(UART_PORT_1);
        }
    }
}
#endif

#if BSP_UART2_ENABLE
void USART2_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        data = USART_ReceiveData(USART2);
        ring_buffer_put(&uart2_rx_buf, data);

        if (uart2_rx_cb != NULL) {
            uart2_rx_cb(UART_PORT_2, &data, 1);
        }
    }

    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) {
        USART_ClearITPendingBit(USART2, USART_IT_TC);
        uart2_tx_busy = 0;
        if (uart2_tx_cb != NULL) {
            uart2_tx_cb(UART_PORT_2);
        }
    }
}
#endif

/*=============================================================================
 *                              printf重定向
 *============================================================================*/

#if BSP_UART_PRINTF_ENABLE
/* 重定向fputc (Keil) */
int fputc(int ch, FILE *f)
{
    (void)f;
    bsp_uart_send_byte(BSP_UART_PRINTF_PORT, (uint8_t)ch);
    return ch;
}
#endif
