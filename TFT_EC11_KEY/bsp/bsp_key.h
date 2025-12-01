/**
  ******************************************************************************
  * @file    bsp_key.h
  * @author  TFT_EC11_KEY Project
  * @brief   独立按键驱动头文件 - 硬件抽象层
  *          支持多按键扫描、长按检测
  ******************************************************************************
  */

#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* 按键配置 ------------------------------------------------------------------*/
#define KEY_NUM_MAX     4    /* 最多支持的按键数量 */

/* 按键扫描周期(ms) */
#define KEY_SCAN_PERIOD_MS      10

/* 长按时间阈值(ms) */
#define KEY_LONG_PRESS_TIME_MS  1000

/* 按键ID定义 ----------------------------------------------------------------*/
typedef enum {
    KEY_ID_0 = 0,
    KEY_ID_1,
    KEY_ID_2,
    KEY_ID_3
} key_id_t;

/* 按键事件类型 --------------------------------------------------------------*/
typedef enum {
    KEY_EVENT_NONE = 0,      /* 无事件 */
    KEY_EVENT_SHORT_PRESS,   /* 短按 */
    KEY_EVENT_LONG_PRESS,    /* 长按 */
    KEY_EVENT_RELEASE        /* 释放 */
} key_event_t;

/* 按键配置结构体 ------------------------------------------------------------*/
typedef struct {
    GPIO_TypeDef *gpio_port;  /* GPIO端口 */
    uint16_t      gpio_pin;   /* GPIO引脚 */
    uint8_t       active_level; /* 有效电平: 0-低电平, 1-高电平 */
} key_config_t;

/* 按键事件回调函数类型 ------------------------------------------------------*/
typedef void (*key_event_callback_t)(key_id_t key_id, key_event_t event);

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  按键模块初始化
 * @param  config: 按键配置数组
 * @param  key_num: 按键数量
 * @retval None
 */
void bsp_key_init(const key_config_t *config, uint8_t key_num);

/**
 * @brief  按键扫描函数 (需要周期调用)
 * @param  None
 * @retval None
 */
void bsp_key_scan(void);

/**
 * @brief  获取按键状态
 * @param  key_id: 按键ID
 * @retval 0-未按下, 1-按下
 */
uint8_t bsp_key_get_state(key_id_t key_id);

/**
 * @brief  注册按键事件回调函数
 * @param  callback: 回调函数指针
 * @retval None
 */
void bsp_key_register_callback(key_event_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_KEY_H */
