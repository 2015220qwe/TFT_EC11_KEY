/**
 * @file bsp_bluetooth.c
 * @brief 蓝牙模块驱动实现 - 支持HC-05/HC-06/BLE模块
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_bluetooth.h"
#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static bt_state_t bt_state = BT_STATE_DISCONNECTED;
static bt_state_t bt_last_state = BT_STATE_DISCONNECTED;

static uint8_t rx_buffer[BT_RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

static bt_rx_callback_t rx_callback = NULL;
static bt_frame_callback_t frame_callback = NULL;
static bt_state_callback_t state_callback = NULL;

/* 帧接收状态机 */
static enum {
    FRAME_STATE_IDLE,
    FRAME_STATE_CMD,
    FRAME_STATE_LEN,
    FRAME_STATE_DATA,
    FRAME_STATE_TAIL
} frame_state = FRAME_STATE_IDLE;

static bt_frame_t rx_frame;
static uint8_t frame_data_idx = 0;

/* AT模式标志 */
static uint8_t in_at_mode = 0;

/* 延时函数 */
extern void delay_ms(uint32_t ms);

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static void bt_gpio_init(void);
static void bt_uart_rx_handler(uart_port_t port, uint8_t *data, uint16_t len);
static void bt_process_frame_byte(uint8_t byte);
static void bt_update_state(void);

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief 蓝牙初始化
 */
int bsp_bluetooth_init(void)
{
    uart_config_t uart_cfg;

    /* 初始化GPIO */
    bt_gpio_init();

    /* 初始化串口 */
    uart_cfg = bsp_uart_get_default_config();
    uart_cfg.baudrate = BT_DEFAULT_BAUD;

    if (bsp_uart_init(BT_UART_PORT, &uart_cfg) != 0) {
        return -1;
    }

    /* 设置接收回调 */
    bsp_uart_set_rx_callback(BT_UART_PORT, bt_uart_rx_handler);

    /* 初始化状态 */
    bt_state = BT_STATE_DISCONNECTED;
    rx_head = 0;
    rx_tail = 0;
    frame_state = FRAME_STATE_IDLE;
    in_at_mode = 0;

    /* EN引脚低电平 (正常工作模式) */
    BT_EN_LOW();

    return 0;
}

/**
 * @brief 蓝牙反初始化
 */
void bsp_bluetooth_deinit(void)
{
    bsp_uart_deinit(BT_UART_PORT);
}

/**
 * @brief 蓝牙复位
 */
void bsp_bluetooth_reset(void)
{
    /* 通过EN引脚复位 (如果支持) */
    BT_EN_HIGH();
    delay_ms(100);
    BT_EN_LOW();
    delay_ms(500);

    bt_state = BT_STATE_DISCONNECTED;
}

/**
 * @brief 周期处理
 */
void bsp_bluetooth_process(void)
{
    /* 更新连接状态 */
    bt_update_state();

    /* 状态变化回调 */
    if (bt_state != bt_last_state) {
        if (state_callback != NULL) {
            state_callback(bt_state);
        }
        bt_last_state = bt_state;
    }
}

/**
 * @brief 获取连接状态
 */
bt_state_t bsp_bluetooth_get_state(void)
{
    return bt_state;
}

/**
 * @brief 是否已连接
 */
uint8_t bsp_bluetooth_is_connected(void)
{
    return (bt_state == BT_STATE_CONNECTED) ? 1 : 0;
}

/**
 * @brief 发送数据
 */
uint16_t bsp_bluetooth_send(const uint8_t *data, uint16_t len)
{
    return bsp_uart_send(BT_UART_PORT, data, len);
}

/**
 * @brief 发送字符串
 */
void bsp_bluetooth_send_string(const char *str)
{
    bsp_uart_send_string(BT_UART_PORT, str);
}

/**
 * @brief 格式化发送
 */
void bsp_bluetooth_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    bsp_bluetooth_send_string(buf);
}

/**
 * @brief 发送数据帧
 */
int bsp_bluetooth_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t frame[BT_FRAME_MAX_DATA + 4];
    uint8_t checksum = 0;
    uint8_t i;

    if (len > BT_FRAME_MAX_DATA) {
        return -1;
    }

    /* 组装帧 */
    frame[0] = BT_FRAME_HEADER;
    frame[1] = cmd;
    frame[2] = len;

    checksum = cmd ^ len;
    for (i = 0; i < len; i++) {
        frame[3 + i] = data[i];
        checksum ^= data[i];
    }
    frame[3 + len] = checksum;
    frame[4 + len] = BT_FRAME_TAIL;

    /* 发送 */
    bsp_bluetooth_send(frame, 5 + len);

    return 0;
}

/**
 * @brief 接收数据
 */
uint16_t bsp_bluetooth_receive(uint8_t *data, uint16_t max_len)
{
    uint16_t count = 0;

    while (rx_tail != rx_head && count < max_len) {
        data[count++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % BT_RX_BUFFER_SIZE;
    }

    return count;
}

/**
 * @brief 获取接收数据量
 */
uint16_t bsp_bluetooth_get_rx_count(void)
{
    return (rx_head - rx_tail + BT_RX_BUFFER_SIZE) % BT_RX_BUFFER_SIZE;
}

/**
 * @brief 清空接收缓冲区
 */
void bsp_bluetooth_flush_rx(void)
{
    rx_head = 0;
    rx_tail = 0;
}

/**
 * @brief 设置数据接收回调
 */
void bsp_bluetooth_set_rx_callback(bt_rx_callback_t callback)
{
    rx_callback = callback;
}

/**
 * @brief 设置帧接收回调
 */
void bsp_bluetooth_set_frame_callback(bt_frame_callback_t callback)
{
    frame_callback = callback;
}

/**
 * @brief 设置状态变化回调
 */
void bsp_bluetooth_set_state_callback(bt_state_callback_t callback)
{
    state_callback = callback;
}

/*=============================================================================
 *                              AT命令函数
 *============================================================================*/

/**
 * @brief 进入AT模式
 */
int bsp_bluetooth_enter_at_mode(void)
{
#if BT_MODULE_TYPE == BT_MODULE_HC05
    /* HC-05: EN拉高进入AT模式 */
    BT_EN_HIGH();
    delay_ms(100);

    /* 切换波特率到38400 (HC-05 AT模式默认波特率) */
    bsp_uart_set_baudrate(BT_UART_PORT, 38400);
    delay_ms(100);

    /* 测试AT通信 */
    if (bsp_bluetooth_test_at() == 0) {
        in_at_mode = 1;
        bt_state = BT_STATE_AT_MODE;
        return 0;
    }

    /* 失败，恢复 */
    BT_EN_LOW();
    bsp_uart_set_baudrate(BT_UART_PORT, BT_DEFAULT_BAUD);
    return -1;

#elif BT_MODULE_TYPE == BT_MODULE_DX_BT311
    /* DX-BT311: EN拉高 + 上电进入AT模式 */
    /* 注意: 实际使用需要断电后按住按键再上电 */
    BT_EN_HIGH();
    delay_ms(200);

    /* 切换波特率到38400 (DX-BT311 AT模式波特率) */
    bsp_uart_set_baudrate(BT_UART_PORT, BT_DX311_AT_BAUD);
    delay_ms(100);

    /* 测试AT通信 */
    if (bsp_bluetooth_test_at() == 0) {
        in_at_mode = 1;
        bt_state = BT_STATE_AT_MODE;
        return 0;
    }

    /* 尝试工作模式波特率 */
    bsp_uart_set_baudrate(BT_UART_PORT, BT_DX311_WORK_BAUD);
    delay_ms(100);

    if (bsp_bluetooth_test_at() == 0) {
        in_at_mode = 1;
        bt_state = BT_STATE_AT_MODE;
        return 0;
    }

    /* 失败，恢复 */
    BT_EN_LOW();
    bsp_uart_set_baudrate(BT_UART_PORT, BT_DEFAULT_BAUD);
    return -1;

#else
    /* HC-06/HM-10等模块在未连接时可直接发AT命令 */
    if (bsp_bluetooth_test_at() == 0) {
        in_at_mode = 1;
        bt_state = BT_STATE_AT_MODE;
        return 0;
    }
    return -1;
#endif
}

/**
 * @brief 退出AT模式
 */
void bsp_bluetooth_exit_at_mode(void)
{
#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    BT_EN_LOW();
    delay_ms(100);
    bsp_uart_set_baudrate(BT_UART_PORT, BT_DEFAULT_BAUD);
#endif
    in_at_mode = 0;
    bt_state = BT_STATE_DISCONNECTED;
}

/**
 * @brief 发送AT命令
 */
int bsp_bluetooth_at_cmd(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout_ms)
{
    char at_cmd[64];
    uint16_t idx = 0;
    uint8_t byte;

    /* 清空接收缓冲区 */
    bsp_uart_flush_rx(BT_UART_PORT);

    /* 发送AT命令 */
#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    snprintf(at_cmd, sizeof(at_cmd), "AT+%s\r\n", cmd);
#else
    snprintf(at_cmd, sizeof(at_cmd), "AT+%s", cmd);
#endif
    bsp_uart_send_string(BT_UART_PORT, at_cmd);

    if (response != NULL && resp_size > 0) {
        response[0] = '\0';

        /* 简单超时等待 */
        delay_ms(timeout_ms);

        /* 读取响应 */
        while (bsp_uart_receive_byte(BT_UART_PORT, &byte) == 0 && idx < resp_size - 1) {
            response[idx++] = byte;
        }
        response[idx] = '\0';

        /* 检查是否包含OK */
        if (strstr(response, "OK") != NULL) {
            return 0;
        } else if (strstr(response, "ERROR") != NULL || strstr(response, "FAIL") != NULL) {
            return -2;
        }
    }

    return -1;
}

/**
 * @brief 测试AT通信
 */
int bsp_bluetooth_test_at(void)
{
    char response[32];

#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    /* HC-05/DX-BT311: AT返回OK */
    bsp_uart_send_string(BT_UART_PORT, "AT\r\n");
#else
    bsp_uart_send_string(BT_UART_PORT, "AT");
#endif

    delay_ms(500);

    uint16_t len = bsp_uart_receive(BT_UART_PORT, (uint8_t*)response, sizeof(response) - 1);
    response[len] = '\0';

    if (strstr(response, "OK") != NULL) {
        return 0;
    }

    return -1;
}

/**
 * @brief 设置蓝牙名称
 */
int bsp_bluetooth_set_name(const char *name)
{
    char cmd[48];

#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    snprintf(cmd, sizeof(cmd), "NAME=%s", name);
#elif BT_MODULE_TYPE == BT_MODULE_HC06
    snprintf(cmd, sizeof(cmd), "NAME%s", name);
#else /* HM-10 BLE */
    snprintf(cmd, sizeof(cmd), "NAME%s", name);
#endif

    return bsp_bluetooth_at_cmd(cmd, NULL, 0, BT_AT_TIMEOUT);
}

/**
 * @brief 设置配对密码
 */
int bsp_bluetooth_set_pin(const char *pin)
{
    char cmd[16];

#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    snprintf(cmd, sizeof(cmd), "PSWD=%s", pin);
#elif BT_MODULE_TYPE == BT_MODULE_HC06
    snprintf(cmd, sizeof(cmd), "PIN%s", pin);
#else
    snprintf(cmd, sizeof(cmd), "PASS%s", pin);
#endif

    return bsp_bluetooth_at_cmd(cmd, NULL, 0, BT_AT_TIMEOUT);
}

/**
 * @brief 设置波特率
 */
int bsp_bluetooth_set_baudrate(uint32_t baudrate)
{
    char cmd[24];
    int ret;

#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    /* HC-05/DX-BT311: UART=波特率,停止位,校验位 */
    snprintf(cmd, sizeof(cmd), "UART=%lu,0,0", baudrate);
#else
    snprintf(cmd, sizeof(cmd), "BAUD%lu", baudrate);
#endif

    ret = bsp_bluetooth_at_cmd(cmd, NULL, 0, BT_AT_TIMEOUT);

    if (ret == 0) {
        /* 更新本地波特率 */
        bsp_uart_set_baudrate(BT_UART_PORT, baudrate);
    }

    return ret;
}

/**
 * @brief 设置角色
 */
int bsp_bluetooth_set_role(bt_role_t role)
{
#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "ROLE=%d", role);
    return bsp_bluetooth_at_cmd(cmd, NULL, 0, BT_AT_TIMEOUT);
#else
    (void)role;
    return -1; /* 不支持 */
#endif
}

/**
 * @brief 恢复出厂设置
 */
int bsp_bluetooth_factory_reset(void)
{
#if BT_MODULE_TYPE == BT_MODULE_HC05
    return bsp_bluetooth_at_cmd("ORGL", NULL, 0, BT_AT_TIMEOUT);
#elif BT_MODULE_TYPE == BT_MODULE_DX_BT311
    return bsp_bluetooth_at_cmd("DEFAULT", NULL, 0, BT_AT_TIMEOUT);
#elif BT_MODULE_TYPE == BT_MODULE_HM10
    return bsp_bluetooth_at_cmd("RENEW", NULL, 0, BT_AT_TIMEOUT);
#else
    return -1;
#endif
}

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

/**
 * @brief GPIO初始化
 */
static void bt_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能时钟 */
    RCC_AHB1PeriphClockCmd(BT_STATE_CLK | BT_EN_CLK, ENABLE);

    /* STATE引脚 - 输入 */
    GPIO_InitStructure.GPIO_Pin = BT_STATE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(BT_STATE_PORT, &GPIO_InitStructure);

    /* EN引脚 - 输出 */
    GPIO_InitStructure.GPIO_Pin = BT_EN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(BT_EN_PORT, &GPIO_InitStructure);

    BT_EN_LOW();
}

/**
 * @brief 串口接收回调
 */
static void bt_uart_rx_handler(uart_port_t port, uint8_t *data, uint16_t len)
{
    uint16_t i;

    (void)port;

    for (i = 0; i < len; i++) {
        /* 存入缓冲区 */
        uint16_t next = (rx_head + 1) % BT_RX_BUFFER_SIZE;
        if (next != rx_tail) {
            rx_buffer[rx_head] = data[i];
            rx_head = next;
        }

        /* 帧解析 */
        if (!in_at_mode) {
            bt_process_frame_byte(data[i]);
        }
    }

    /* 原始数据回调 */
    if (rx_callback != NULL && !in_at_mode) {
        rx_callback(data, len);
    }
}

/**
 * @brief 帧解析状态机
 */
static void bt_process_frame_byte(uint8_t byte)
{
    switch (frame_state) {
    case FRAME_STATE_IDLE:
        if (byte == BT_FRAME_HEADER) {
            frame_state = FRAME_STATE_CMD;
        }
        break;

    case FRAME_STATE_CMD:
        rx_frame.cmd = byte;
        frame_state = FRAME_STATE_LEN;
        break;

    case FRAME_STATE_LEN:
        rx_frame.len = byte;
        if (rx_frame.len > BT_FRAME_MAX_DATA) {
            frame_state = FRAME_STATE_IDLE;
        } else if (rx_frame.len == 0) {
            frame_state = FRAME_STATE_TAIL;
        } else {
            frame_data_idx = 0;
            frame_state = FRAME_STATE_DATA;
        }
        break;

    case FRAME_STATE_DATA:
        rx_frame.data[frame_data_idx++] = byte;
        if (frame_data_idx >= rx_frame.len) {
            frame_state = FRAME_STATE_TAIL;
        }
        break;

    case FRAME_STATE_TAIL:
        if (byte == BT_FRAME_TAIL) {
            /* 帧接收完成 */
            if (frame_callback != NULL) {
                frame_callback(&rx_frame);
            }
        }
        frame_state = FRAME_STATE_IDLE;
        break;
    }
}

/**
 * @brief 更新连接状态
 */
static void bt_update_state(void)
{
    if (in_at_mode) {
        bt_state = BT_STATE_AT_MODE;
        return;
    }

#if BT_MODULE_TYPE == BT_MODULE_HC05 || BT_MODULE_TYPE == BT_MODULE_DX_BT311
    /* HC-05/DX-BT311: STATE引脚高电平表示已连接 */
    if (BT_GET_STATE()) {
        bt_state = BT_STATE_CONNECTED;
    } else {
        bt_state = BT_STATE_DISCONNECTED;
    }
#else
    /* 其他模块可能需要不同的检测方式 */
    /* 简单方案：通过数据收发判断 */
#endif
}
