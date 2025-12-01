/**
  ******************************************************************************
  * @file    bsp_ec11.h
  * @author  TFT_EC11_KEY Project
  * @brief   EC11旋转编码器驱动头文件 - 硬件抽象层
  *          适用于STM32F407VGT6 (立创梁山派天空星开发板)
  *          支持标准库和HAL库两种实现
  ******************************************************************************
  * @attention
  *
  * 本驱动提供EC11旋转编码器的完整功能:
  * - 左旋/右旋检测
  * - 按键检测(短按/长按)
  * - 硬件消抖
  * - 事件回调机制
  *
  * 移植说明:
  * 1. 根据使用的库修改 BSP_EC11_USE_HAL 宏定义
  * 2. 修改引脚定义 (EC11_A_PIN, EC11_B_PIN, EC11_KEY_PIN)
  * 3. 在中断服务函数中调用 bsp_ec11_exti_callback()
  * 4. 在主循环或定时器中调用 bsp_ec11_scan()
  *
  ******************************************************************************
  */

#ifndef __BSP_EC11_H
#define __BSP_EC11_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* 配置选项 ------------------------------------------------------------------*/
/* 如果使用HAL库，请定义此宏；如果使用标准库，请注释掉 */
// #define BSP_EC11_USE_HAL

/* EC11硬件引脚定义 ----------------------------------------------------------*/
/*
 * 默认引脚配置 (可根据实际硬件修改):
 * EC11_A    -> PA0  (编码器A相)
 * EC11_B    -> PA1  (编码器B相)
 * EC11_KEY  -> PA2  (编码器按键)
 */
#define EC11_A_GPIO_PORT        GPIOA
#define EC11_A_GPIO_PIN         GPIO_Pin_0
#define EC11_A_GPIO_CLK         RCC_AHB1Periph_GPIOA
#define EC11_A_EXTI_LINE        EXTI_Line0
#define EC11_A_EXTI_PORT_SOURCE EXTI_PortSourceGPIOA
#define EC11_A_EXTI_PIN_SOURCE  EXTI_PinSource0
#define EC11_A_EXTI_IRQn        EXTI0_IRQn

#define EC11_B_GPIO_PORT        GPIOA
#define EC11_B_GPIO_PIN         GPIO_Pin_1
#define EC11_B_GPIO_CLK         RCC_AHB1Periph_GPIOA

#define EC11_KEY_GPIO_PORT      GPIOA
#define EC11_KEY_GPIO_PIN       GPIO_Pin_2
#define EC11_KEY_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define EC11_KEY_EXTI_LINE      EXTI_Line2
#define EC11_KEY_EXTI_PORT_SOURCE EXTI_PortSourceGPIOA
#define EC11_KEY_EXTI_PIN_SOURCE  EXTI_PinSource2
#define EC11_KEY_EXTI_IRQn      EXTI2_IRQn

/* 消抖和长按时间配置 --------------------------------------------------------*/
#define EC11_DEBOUNCE_TIME_MS   20    /* 消抖时间(毫秒) */
#define EC11_LONG_PRESS_TIME_MS 1000  /* 长按判定时间(毫秒) */

/* EC11事件类型定义 ----------------------------------------------------------*/
typedef enum {
    EC11_EVENT_NONE = 0,        /* 无事件 */
    EC11_EVENT_ROTATE_LEFT,     /* 左旋 */
    EC11_EVENT_ROTATE_RIGHT,    /* 右旋 */
    EC11_EVENT_KEY_SHORT_PRESS, /* 短按 */
    EC11_EVENT_KEY_LONG_PRESS,  /* 长按 */
    EC11_EVENT_KEY_RELEASE      /* 按键释放 */
} ec11_event_t;

/* EC11状态结构体 ------------------------------------------------------------*/
typedef struct {
    int32_t  count;              /* 旋转计数器 (左旋减，右旋加) */
    uint8_t  key_state;          /* 按键状态: 0-释放, 1-按下 */
    uint32_t key_press_time;     /* 按键按下时间戳(ms) */
    uint8_t  last_a_state;       /* 上一次A相状态 */
    uint8_t  last_b_state;       /* 上一次B相状态 */
} ec11_state_t;

/* 事件回调函数类型 ----------------------------------------------------------*/
typedef void (*ec11_event_callback_t)(ec11_event_t event);

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  EC11编码器初始化
 * @param  None
 * @retval None
 */
void bsp_ec11_init(void);

/**
 * @brief  EC11编码器扫描函数 (需要在主循环或定时器中周期调用)
 * @param  None
 * @retval 返回当前事件
 */
ec11_event_t bsp_ec11_scan(void);

/**
 * @brief  获取EC11旋转计数值
 * @param  None
 * @retval 当前计数值 (左旋递减, 右旋递增)
 */
int32_t bsp_ec11_get_count(void);

/**
 * @brief  重置EC11旋转计数值
 * @param  count: 要设置的计数值
 * @retval None
 */
void bsp_ec11_set_count(int32_t count);

/**
 * @brief  获取EC11按键状态
 * @param  None
 * @retval 0-未按下, 1-按下
 */
uint8_t bsp_ec11_get_key_state(void);

/**
 * @brief  注册事件回调函数
 * @param  callback: 回调函数指针
 * @retval None
 */
void bsp_ec11_register_callback(ec11_event_callback_t callback);

/**
 * @brief  EXTI中断回调函数 (需要在中断服务函数中调用)
 * @param  exti_line: 中断线
 * @retval None
 */
void bsp_ec11_exti_callback(uint32_t exti_line);

/**
 * @brief  获取系统时间戳 (毫秒) - 需要用户实现
 * @param  None
 * @retval 系统运行时间(毫秒)
 * @note   此函数为弱定义，用户需要根据自己的系统提供实现
 */
uint32_t bsp_ec11_get_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_EC11_H */
