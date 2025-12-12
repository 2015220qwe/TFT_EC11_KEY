/**
 * @file bsp_bluetooth.h
 * @brief 蓝牙模块驱动 - 支持HC-05/HC-06/BLE模块
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 功能特性:
 *       - 支持HC-05/HC-06 SPP蓝牙
 *       - 支持HM-10/CC2541 BLE蓝牙
 *       - AT命令配置
 *       - 透传模式
 *       - 自动重连
 *       - 连接状态检测
 *       - 数据帧协议
 */

#ifndef __BSP_BLUETOOTH_H
#define __BSP_BLUETOOTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* 蓝牙模块类型 */
#define BT_MODULE_HC05          0
#define BT_MODULE_HC06          1
#define BT_MODULE_HM10          2   /* BLE */

/* 当前使用的模块类型 */
#define BT_MODULE_TYPE          BT_MODULE_HC05

/* 蓝牙使用的串口 */
#define BT_UART_PORT            UART_PORT_2

/* 缓冲区大小 */
#define BT_RX_BUFFER_SIZE       256
#define BT_TX_BUFFER_SIZE       256

/* 默认波特率 */
#define BT_DEFAULT_BAUD         9600

/* AT命令超时 (ms) */
#define BT_AT_TIMEOUT           1000

/* 数据帧配置 */
#define BT_FRAME_HEADER         0xAA
#define BT_FRAME_TAIL           0x55
#define BT_FRAME_MAX_DATA       200

/*=============================================================================
 *                              引脚配置
 *============================================================================*/

/* HC-05状态引脚 */
#define BT_STATE_PORT           GPIOA
#define BT_STATE_PIN            GPIO_Pin_4
#define BT_STATE_CLK            RCC_AHB1Periph_GPIOA

/* HC-05 EN引脚 (进入AT模式) */
#define BT_EN_PORT              GPIOA
#define BT_EN_PIN               GPIO_Pin_5
#define BT_EN_CLK               RCC_AHB1Periph_GPIOA

/* 控制宏 */
#define BT_EN_HIGH()            GPIO_SetBits(BT_EN_PORT, BT_EN_PIN)
#define BT_EN_LOW()             GPIO_ResetBits(BT_EN_PORT, BT_EN_PIN)
#define BT_GET_STATE()          GPIO_ReadInputDataBit(BT_STATE_PORT, BT_STATE_PIN)

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief 蓝牙状态
 */
typedef enum {
    BT_STATE_DISCONNECTED = 0,  /**< 未连接 */
    BT_STATE_CONNECTING,        /**< 连接中 */
    BT_STATE_CONNECTED,         /**< 已连接 */
    BT_STATE_AT_MODE            /**< AT命令模式 */
} bt_state_t;

/**
 * @brief 蓝牙角色 (HC-05)
 */
typedef enum {
    BT_ROLE_SLAVE = 0,          /**< 从机 */
    BT_ROLE_MASTER,             /**< 主机 */
    BT_ROLE_SLAVE_LOOP          /**< 从机回环 */
} bt_role_t;

/**
 * @brief 蓝牙配置
 */
typedef struct {
    char name[32];              /**< 蓝牙名称 */
    char pin[8];                /**< 配对密码 */
    uint32_t baudrate;          /**< 波特率 */
    bt_role_t role;             /**< 角色 */
} bt_config_t;

/**
 * @brief 数据帧结构
 */
typedef struct {
    uint8_t cmd;                /**< 命令码 */
    uint8_t data[BT_FRAME_MAX_DATA];  /**< 数据 */
    uint8_t len;                /**< 数据长度 */
} bt_frame_t;

/**
 * @brief 接收回调
 * @param data 数据
 * @param len 长度
 */
typedef void (*bt_rx_callback_t)(uint8_t *data, uint16_t len);

/**
 * @brief 帧接收回调
 * @param frame 数据帧
 */
typedef void (*bt_frame_callback_t)(const bt_frame_t *frame);

/**
 * @brief 连接状态变化回调
 * @param state 新状态
 */
typedef void (*bt_state_callback_t)(bt_state_t state);

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/*----------------------- 初始化函数 -----------------------*/

/**
 * @brief 蓝牙初始化
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_init(void);

/**
 * @brief 蓝牙反初始化
 */
void bsp_bluetooth_deinit(void);

/**
 * @brief 蓝牙复位
 */
void bsp_bluetooth_reset(void);

/**
 * @brief 周期处理 (主循环调用)
 * @note 处理接收数据和状态更新
 */
void bsp_bluetooth_process(void);

/*----------------------- 状态函数 -----------------------*/

/**
 * @brief 获取连接状态
 * @retval 蓝牙状态
 */
bt_state_t bsp_bluetooth_get_state(void);

/**
 * @brief 是否已连接
 * @retval 1:已连接 0:未连接
 */
uint8_t bsp_bluetooth_is_connected(void);

/**
 * @brief 获取信号强度 (BLE)
 * @retval RSSI值
 */
int8_t bsp_bluetooth_get_rssi(void);

/*----------------------- 数据收发 -----------------------*/

/**
 * @brief 发送数据 (透传模式)
 * @param data 数据
 * @param len 长度
 * @retval 发送长度
 */
uint16_t bsp_bluetooth_send(const uint8_t *data, uint16_t len);

/**
 * @brief 发送字符串
 * @param str 字符串
 */
void bsp_bluetooth_send_string(const char *str);

/**
 * @brief 格式化发送
 */
void bsp_bluetooth_printf(const char *fmt, ...);

/**
 * @brief 发送数据帧
 * @param cmd 命令码
 * @param data 数据
 * @param len 数据长度
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len);

/**
 * @brief 接收数据
 * @param data 缓冲区
 * @param max_len 最大长度
 * @retval 接收长度
 */
uint16_t bsp_bluetooth_receive(uint8_t *data, uint16_t max_len);

/**
 * @brief 获取接收数据量
 * @retval 数据量
 */
uint16_t bsp_bluetooth_get_rx_count(void);

/**
 * @brief 清空接收缓冲区
 */
void bsp_bluetooth_flush_rx(void);

/*----------------------- 回调设置 -----------------------*/

/**
 * @brief 设置数据接收回调
 * @param callback 回调函数
 */
void bsp_bluetooth_set_rx_callback(bt_rx_callback_t callback);

/**
 * @brief 设置帧接收回调
 * @param callback 回调函数
 */
void bsp_bluetooth_set_frame_callback(bt_frame_callback_t callback);

/**
 * @brief 设置状态变化回调
 * @param callback 回调函数
 */
void bsp_bluetooth_set_state_callback(bt_state_callback_t callback);

/*----------------------- AT命令 (HC-05/HC-06) -----------------------*/

/**
 * @brief 进入AT模式
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_enter_at_mode(void);

/**
 * @brief 退出AT模式
 */
void bsp_bluetooth_exit_at_mode(void);

/**
 * @brief 发送AT命令
 * @param cmd AT命令 (不含"AT+"前缀)
 * @param response 响应缓冲区
 * @param resp_size 缓冲区大小
 * @param timeout_ms 超时时间
 * @retval 0:成功 -1:超时 -2:错误
 */
int bsp_bluetooth_at_cmd(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout_ms);

/**
 * @brief 测试AT通信
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_test_at(void);

/**
 * @brief 设置蓝牙名称
 * @param name 名称
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_set_name(const char *name);

/**
 * @brief 获取蓝牙名称
 * @param name 名称缓冲区
 * @param size 缓冲区大小
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_get_name(char *name, uint16_t size);

/**
 * @brief 设置配对密码
 * @param pin 密码 (4位数字)
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_set_pin(const char *pin);

/**
 * @brief 设置波特率
 * @param baudrate 波特率
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_set_baudrate(uint32_t baudrate);

/**
 * @brief 设置角色 (HC-05)
 * @param role 角色
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_set_role(bt_role_t role);

/**
 * @brief 获取蓝牙地址
 * @param addr 地址缓冲区 (格式: XX:XX:XX:XX:XX:XX)
 * @param size 缓冲区大小
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_get_address(char *addr, uint16_t size);

/**
 * @brief 恢复出厂设置
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_factory_reset(void);

/**
 * @brief 应用配置
 * @param config 配置结构体
 * @retval 0:成功 -1:失败
 */
int bsp_bluetooth_apply_config(const bt_config_t *config);

/*=============================================================================
 *                              预定义命令码
 *============================================================================*/

/* 通用命令 */
#define BT_CMD_HEARTBEAT        0x00    /**< 心跳 */
#define BT_CMD_ACK              0x01    /**< 应答 */
#define BT_CMD_ERROR            0x02    /**< 错误 */

/* 数据命令 */
#define BT_CMD_ADC_DATA         0x10    /**< ADC数据 */
#define BT_CMD_WAVEFORM         0x11    /**< 波形数据 */
#define BT_CMD_MENU_STATE       0x12    /**< 菜单状态 */

/* 控制命令 */
#define BT_CMD_SET_PARAM        0x20    /**< 设置参数 */
#define BT_CMD_GET_PARAM        0x21    /**< 获取参数 */
#define BT_CMD_START            0x22    /**< 开始 */
#define BT_CMD_STOP             0x23    /**< 停止 */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_BLUETOOTH_H */
