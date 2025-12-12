/**
 * @file bsp_adc_hal.h
 * @brief ADC驱动模块 - HAL库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 此文件为HAL库版本，需要STM32CubeMX生成的HAL库支持
 * @note 移植时需要:
 *       1. 包含正确的HAL库头文件
 *       2. 配置CubeMX中的ADC外设
 *       3. 实现或修改引脚配置
 */

#ifndef __BSP_ADC_HAL_H
#define __BSP_ADC_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* HAL库版本 - 需要包含对应的HAL头文件 */
#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* ADC参考电压 (mV) */
#define BSP_ADC_VREF_MV             3300

/* ADC分辨率 */
#define BSP_ADC_RESOLUTION          4096

/* 最大通道数 */
#define BSP_ADC_MAX_CHANNELS        8

/* 波形缓冲区大小 */
#define BSP_ADC_WAVEFORM_BUFFER_SIZE    256

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief ADC通道配置
 */
typedef struct {
    uint32_t channel;           /**< ADC通道 (ADC_CHANNEL_x) */
    uint32_t sample_time;       /**< 采样时间 */
} adc_hal_channel_config_t;

/**
 * @brief ADC状态
 */
typedef struct {
    uint8_t is_initialized;
    uint8_t is_running;
    uint16_t last_value;
    uint32_t last_voltage_mv;
} adc_hal_state_t;

/**
 * @brief DMA完成回调
 */
typedef void (*adc_hal_complete_callback_t)(uint16_t *data, uint32_t len);

/*=============================================================================
 *                              函数声明
 *============================================================================*/

#ifdef USE_HAL_DRIVER

/**
 * @brief ADC初始化 (HAL版本)
 * @param hadc ADC句柄指针
 * @retval 0:成功 -1:失败
 */
int bsp_adc_hal_init(ADC_HandleTypeDef *hadc);

/**
 * @brief ADC反初始化
 * @param hadc ADC句柄指针
 */
void bsp_adc_hal_deinit(ADC_HandleTypeDef *hadc);

/**
 * @brief 单次ADC采样
 * @param hadc ADC句柄指针
 * @retval ADC原始值 (0~4095)
 */
uint16_t bsp_adc_hal_read(ADC_HandleTypeDef *hadc);

/**
 * @brief 读取指定通道
 * @param hadc ADC句柄指针
 * @param channel 通道号
 * @retval ADC原始值
 */
uint16_t bsp_adc_hal_read_channel(ADC_HandleTypeDef *hadc, uint32_t channel);

/**
 * @brief 启动DMA采样
 * @param hadc ADC句柄指针
 * @param buffer 数据缓冲区
 * @param length 缓冲区长度
 * @retval 0:成功 -1:失败
 */
int bsp_adc_hal_start_dma(ADC_HandleTypeDef *hadc, uint16_t *buffer, uint32_t length);

/**
 * @brief 停止DMA采样
 * @param hadc ADC句柄指针
 */
void bsp_adc_hal_stop_dma(ADC_HandleTypeDef *hadc);

/**
 * @brief ADC值转换为电压
 * @param raw_value ADC原始值
 * @retval 电压值(mV)
 */
uint32_t bsp_adc_hal_to_voltage(uint16_t raw_value);

/**
 * @brief 读取内部温度
 * @param hadc ADC句柄指针
 * @retval 温度值(0.1℃)
 */
int16_t bsp_adc_hal_read_temperature(ADC_HandleTypeDef *hadc);

/**
 * @brief 多次采样取平均
 * @param hadc ADC句柄指针
 * @param times 采样次数
 * @retval 平均值
 */
uint16_t bsp_adc_hal_read_average(ADC_HandleTypeDef *hadc, uint8_t times);

/**
 * @brief 设置DMA完成回调
 * @param callback 回调函数
 */
void bsp_adc_hal_set_callback(adc_hal_complete_callback_t callback);

/**
 * @brief DMA完成回调 (在HAL回调中调用)
 * @param hadc ADC句柄指针
 */
void bsp_adc_hal_conv_complete_callback(ADC_HandleTypeDef *hadc);

#endif /* USE_HAL_DRIVER */

/*=============================================================================
 *                              通用宏 (HAL/标准库通用)
 *============================================================================*/

/**
 * @brief ADC原始值转电压 (mV)
 */
#define ADC_RAW_TO_MV(raw)      ((uint32_t)(raw) * BSP_ADC_VREF_MV / BSP_ADC_RESOLUTION)

/**
 * @brief 电压转ADC原始值
 */
#define MV_TO_ADC_RAW(mv)       ((uint16_t)((mv) * BSP_ADC_RESOLUTION / BSP_ADC_VREF_MV))

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADC_HAL_H */
