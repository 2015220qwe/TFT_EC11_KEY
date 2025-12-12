/**
 * @file bsp_timer.h
 * @brief 定时器驱动模块 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 硬件平台: STM32F407VGT6
 * @note 功能特性:
 *       - 基本定时中断
 *       - 微秒级精确延时
 *       - 时间戳功能
 *       - 输入捕获
 *       - 超时检测
 */

#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* 系统时钟频率 */
#define TIMER_SYSCLK_MHZ        168

/* 基本定时器配置 (用于时间戳) */
#define TIMER_TIMESTAMP_TIM     TIM6
#define TIMER_TIMESTAMP_CLK     RCC_APB1Periph_TIM6
#define TIMER_TIMESTAMP_IRQn    TIM6_DAC_IRQn

/* 微秒定时器配置 */
#define TIMER_US_TIM            TIM7
#define TIMER_US_CLK            RCC_APB1Periph_TIM7

/* 周期定时器配置 */
#define TIMER_PERIODIC_TIM      TIM2
#define TIMER_PERIODIC_CLK      RCC_APB1Periph_TIM2
#define TIMER_PERIODIC_IRQn     TIM2_IRQn

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief 定时器回调函数类型
 */
typedef void (*timer_callback_t)(void);

/**
 * @brief 超时回调函数类型
 * @param id 超时ID
 */
typedef void (*timeout_callback_t)(uint8_t id);

/**
 * @brief 定时器通道
 */
typedef enum {
    TIMER_CHANNEL_0 = 0,
    TIMER_CHANNEL_1,
    TIMER_CHANNEL_2,
    TIMER_CHANNEL_3,
    TIMER_CHANNEL_MAX
} timer_channel_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/*----------------------- 初始化函数 -----------------------*/

/**
 * @brief 定时器模块初始化
 * @note 初始化时间戳和微秒延时定时器
 */
void bsp_timer_init(void);

/**
 * @brief 定时器模块反初始化
 */
void bsp_timer_deinit(void);

/*----------------------- 时间戳函数 -----------------------*/

/**
 * @brief 获取当前时间戳 (ms)
 * @retval 时间戳 (ms)
 */
uint32_t bsp_timer_get_ms(void);

/**
 * @brief 获取当前时间戳 (us)
 * @retval 时间戳 (us)
 */
uint32_t bsp_timer_get_us(void);

/**
 * @brief 时间戳中断处理 (在TIM6中断中调用)
 */
void bsp_timer_timestamp_isr(void);

/*----------------------- 延时函数 -----------------------*/

/**
 * @brief 微秒级阻塞延时
 * @param us 延时时间 (us)
 */
void bsp_timer_delay_us(uint32_t us);

/**
 * @brief 毫秒级阻塞延时
 * @param ms 延时时间 (ms)
 */
void bsp_timer_delay_ms(uint32_t ms);

/*----------------------- 周期定时器 -----------------------*/

/**
 * @brief 启动周期定时器
 * @param period_us 周期 (us)
 * @param callback 回调函数
 * @retval 0:成功 -1:失败
 */
int bsp_timer_start_periodic(uint32_t period_us, timer_callback_t callback);

/**
 * @brief 停止周期定时器
 */
void bsp_timer_stop_periodic(void);

/**
 * @brief 设置周期定时器周期
 * @param period_us 新周期 (us)
 */
void bsp_timer_set_periodic_period(uint32_t period_us);

/**
 * @brief 周期定时器中断处理 (在TIM2中断中调用)
 */
void bsp_timer_periodic_isr(void);

/*----------------------- 单次定时器 -----------------------*/

/**
 * @brief 启动单次定时器
 * @param channel 通道
 * @param delay_ms 延时 (ms)
 * @param callback 回调函数
 * @retval 0:成功 -1:失败
 */
int bsp_timer_start_oneshot(timer_channel_t channel, uint32_t delay_ms, timer_callback_t callback);

/**
 * @brief 取消单次定时器
 * @param channel 通道
 */
void bsp_timer_cancel_oneshot(timer_channel_t channel);

/**
 * @brief 单次定时器检查 (主循环调用)
 */
void bsp_timer_oneshot_process(void);

/*----------------------- 超时检测 -----------------------*/

/**
 * @brief 启动超时检测
 * @param id 超时ID (0~7)
 * @param timeout_ms 超时时间 (ms)
 * @param callback 超时回调
 * @retval 0:成功 -1:失败
 */
int bsp_timer_timeout_start(uint8_t id, uint32_t timeout_ms, timeout_callback_t callback);

/**
 * @brief 喂狗 (重置超时)
 * @param id 超时ID
 */
void bsp_timer_timeout_feed(uint8_t id);

/**
 * @brief 停止超时检测
 * @param id 超时ID
 */
void bsp_timer_timeout_stop(uint8_t id);

/**
 * @brief 超时检测处理 (主循环调用)
 */
void bsp_timer_timeout_process(void);

/*----------------------- 时间测量 -----------------------*/

/**
 * @brief 开始测量
 */
void bsp_timer_measure_start(void);

/**
 * @brief 结束测量
 * @retval 测量时间 (us)
 */
uint32_t bsp_timer_measure_stop(void);

/*----------------------- 运行时间 -----------------------*/

/**
 * @brief 获取系统运行时间 (秒)
 * @retval 运行时间 (秒)
 */
uint32_t bsp_timer_get_uptime_sec(void);

/**
 * @brief 获取格式化的运行时间
 * @param hours 小时
 * @param minutes 分钟
 * @param seconds 秒
 */
void bsp_timer_get_uptime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TIMER_H */
