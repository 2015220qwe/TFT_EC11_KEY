/**
  ******************************************************************************
  * @file    bsp_ec11_hal.h
  * @author  TFT_EC11_KEY Project
  * @brief   EC11旋转编码器驱动 - STM32 HAL库版本
  ******************************************************************************
  */

#ifndef __BSP_EC11_HAL_H
#define __BSP_EC11_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* EC11硬件引脚定义(HAL库) --------------------------------------------------*/
#define EC11_A_GPIO_PORT        GPIOA
#define EC11_A_GPIO_PIN         GPIO_PIN_0
#define EC11_A_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define EC11_A_EXTI_IRQn        EXTI0_IRQn

#define EC11_B_GPIO_PORT        GPIOA
#define EC11_B_GPIO_PIN         GPIO_PIN_1
#define EC11_B_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

#define EC11_KEY_GPIO_PORT      GPIOA
#define EC11_KEY_GPIO_PIN       GPIO_PIN_2
#define EC11_KEY_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define EC11_KEY_EXTI_IRQn      EXTI2_IRQn

/* 复用标准库版本的事件定义 */
typedef enum {
    EC11_EVENT_NONE = 0,
    EC11_EVENT_ROTATE_LEFT,
    EC11_EVENT_ROTATE_RIGHT,
    EC11_EVENT_KEY_SHORT_PRESS,
    EC11_EVENT_KEY_LONG_PRESS,
    EC11_EVENT_KEY_RELEASE
} ec11_event_t;

/* 事件回调函数类型 */
typedef void (*ec11_event_callback_t)(ec11_event_t event);

/* API函数声明 ---------------------------------------------------------------*/
void bsp_ec11_hal_init(void);
ec11_event_t bsp_ec11_hal_scan(void);
int32_t bsp_ec11_hal_get_count(void);
void bsp_ec11_hal_set_count(int32_t count);
uint8_t bsp_ec11_hal_get_key_state(void);
void bsp_ec11_hal_register_callback(ec11_event_callback_t callback);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_EC11_HAL_H */
