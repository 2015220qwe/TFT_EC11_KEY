/**
 * @file bsp_pwm.h
 * @brief PWM输出驱动模块 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 硬件平台: STM32F407VGT6
 * @note 功能特性:
 *       - 多通道PWM输出
 *       - 频率/占空比可调
 *       - 支持互补输出
 *       - 支持死区时间
 *       - 渐变效果
 *       - 呼吸灯效果
 */

#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* 最大PWM通道数 */
#define PWM_MAX_CHANNELS        8

/* 默认PWM频率 */
#define PWM_DEFAULT_FREQ        1000    /* 1kHz */

/* 默认分辨率 */
#define PWM_DEFAULT_RESOLUTION  1000    /* 0.1%分辨率 */

/*=============================================================================
 *                              预定义通道配置
 *============================================================================*/

/* TIM3 CH1 - PA6 */
#define PWM_TIM3_CH1_PORT       GPIOA
#define PWM_TIM3_CH1_PIN        GPIO_Pin_6
#define PWM_TIM3_CH1_PINSRC     GPIO_PinSource6
#define PWM_TIM3_CH1_AF         GPIO_AF_TIM3

/* TIM3 CH2 - PA7 */
#define PWM_TIM3_CH2_PORT       GPIOA
#define PWM_TIM3_CH2_PIN        GPIO_Pin_7
#define PWM_TIM3_CH2_PINSRC     GPIO_PinSource7
#define PWM_TIM3_CH2_AF         GPIO_AF_TIM3

/* TIM4 CH1 - PB6 */
#define PWM_TIM4_CH1_PORT       GPIOB
#define PWM_TIM4_CH1_PIN        GPIO_Pin_6
#define PWM_TIM4_CH1_PINSRC     GPIO_PinSource6
#define PWM_TIM4_CH1_AF         GPIO_AF_TIM4

/* TIM4 CH2 - PB7 */
#define PWM_TIM4_CH2_PORT       GPIOB
#define PWM_TIM4_CH2_PIN        GPIO_Pin_7
#define PWM_TIM4_CH2_PINSRC     GPIO_PinSource7
#define PWM_TIM4_CH2_AF         GPIO_AF_TIM4

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief PWM通道ID
 */
typedef uint8_t pwm_channel_t;

/**
 * @brief PWM定时器枚举
 */
typedef enum {
    PWM_TIMER_1 = 0,
    PWM_TIMER_2,
    PWM_TIMER_3,
    PWM_TIMER_4,
    PWM_TIMER_5,
    PWM_TIMER_8,
    PWM_TIMER_COUNT
} pwm_timer_t;

/**
 * @brief PWM通道枚举
 */
typedef enum {
    PWM_CH_1 = 0,
    PWM_CH_2,
    PWM_CH_3,
    PWM_CH_4
} pwm_ch_t;

/**
 * @brief PWM配置结构体
 */
typedef struct {
    pwm_timer_t timer;              /**< 定时器 */
    pwm_ch_t channel;               /**< 通道 */
    GPIO_TypeDef *gpio_port;        /**< GPIO端口 */
    uint16_t gpio_pin;              /**< GPIO引脚 */
    uint8_t gpio_pinsrc;            /**< GPIO引脚源 */
    uint8_t gpio_af;                /**< GPIO复用功能 */
    uint32_t frequency;             /**< 频率 (Hz) */
    uint16_t resolution;            /**< 分辨率 (占空比范围0~resolution) */
} pwm_config_t;

/**
 * @brief 呼吸灯参数
 */
typedef struct {
    uint16_t min_duty;              /**< 最小占空比 */
    uint16_t max_duty;              /**< 最大占空比 */
    uint16_t step;                  /**< 步进值 */
    uint16_t interval_ms;           /**< 步进间隔 (ms) */
} pwm_breath_param_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief PWM初始化
 * @param config 配置参数
 * @retval 通道ID，失败返回0xFF
 */
pwm_channel_t bsp_pwm_init(const pwm_config_t *config);

/**
 * @brief PWM反初始化
 * @param channel 通道ID
 */
void bsp_pwm_deinit(pwm_channel_t channel);

/**
 * @brief 设置占空比
 * @param channel 通道ID
 * @param duty 占空比 (0 ~ resolution)
 */
void bsp_pwm_set_duty(pwm_channel_t channel, uint16_t duty);

/**
 * @brief 设置占空比 (百分比)
 * @param channel 通道ID
 * @param percent 占空比 (0.0 ~ 100.0)
 */
void bsp_pwm_set_duty_percent(pwm_channel_t channel, float percent);

/**
 * @brief 获取占空比
 * @param channel 通道ID
 * @retval 占空比
 */
uint16_t bsp_pwm_get_duty(pwm_channel_t channel);

/**
 * @brief 设置频率
 * @param channel 通道ID
 * @param freq 频率 (Hz)
 * @retval 0:成功 -1:失败
 */
int bsp_pwm_set_frequency(pwm_channel_t channel, uint32_t freq);

/**
 * @brief 获取频率
 * @param channel 通道ID
 * @retval 频率 (Hz)
 */
uint32_t bsp_pwm_get_frequency(pwm_channel_t channel);

/**
 * @brief 启动PWM输出
 * @param channel 通道ID
 */
void bsp_pwm_start(pwm_channel_t channel);

/**
 * @brief 停止PWM输出
 * @param channel 通道ID
 */
void bsp_pwm_stop(pwm_channel_t channel);

/**
 * @brief 渐变到目标占空比
 * @param channel 通道ID
 * @param target_duty 目标占空比
 * @param duration_ms 渐变时间 (ms)
 */
void bsp_pwm_fade_to(pwm_channel_t channel, uint16_t target_duty, uint32_t duration_ms);

/**
 * @brief 启动呼吸灯效果
 * @param channel 通道ID
 * @param param 呼吸参数 (NULL使用默认)
 */
void bsp_pwm_breath_start(pwm_channel_t channel, const pwm_breath_param_t *param);

/**
 * @brief 停止呼吸灯效果
 * @param channel 通道ID
 */
void bsp_pwm_breath_stop(pwm_channel_t channel);

/**
 * @brief 呼吸灯更新 (主循环调用)
 */
void bsp_pwm_breath_update(void);

/**
 * @brief 获取预定义配置
 * @param timer 定时器
 * @param ch 通道
 * @retval 配置结构体
 */
pwm_config_t bsp_pwm_get_preset_config(pwm_timer_t timer, pwm_ch_t ch);

/*=============================================================================
 *                              便捷宏
 *============================================================================*/

/* 快速初始化TIM3 CH1 (PA6) */
#define PWM_INIT_TIM3_CH1(freq) \
    bsp_pwm_init(&(pwm_config_t){ \
        .timer = PWM_TIMER_3, .channel = PWM_CH_1, \
        .gpio_port = PWM_TIM3_CH1_PORT, .gpio_pin = PWM_TIM3_CH1_PIN, \
        .gpio_pinsrc = PWM_TIM3_CH1_PINSRC, .gpio_af = PWM_TIM3_CH1_AF, \
        .frequency = freq, .resolution = PWM_DEFAULT_RESOLUTION \
    })

/* 快速初始化TIM4 CH1 (PB6) */
#define PWM_INIT_TIM4_CH1(freq) \
    bsp_pwm_init(&(pwm_config_t){ \
        .timer = PWM_TIMER_4, .channel = PWM_CH_1, \
        .gpio_port = PWM_TIM4_CH1_PORT, .gpio_pin = PWM_TIM4_CH1_PIN, \
        .gpio_pinsrc = PWM_TIM4_CH1_PINSRC, .gpio_af = PWM_TIM4_CH1_AF, \
        .frequency = freq, .resolution = PWM_DEFAULT_RESOLUTION \
    })

#ifdef __cplusplus
}
#endif

#endif /* __BSP_PWM_H */
