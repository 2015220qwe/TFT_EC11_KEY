/**
 * @file bsp_adc_hal.c
 * @brief ADC驱动 - HAL库版本实现
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#ifdef USE_HAL_DRIVER

#include "bsp_hal/bsp_adc_hal.h"
#include <string.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static ADC_HandleTypeDef *hadc_instance = NULL;
static adc_hal_callback_t dma_callback = NULL;
static uint16_t *dma_buffer = NULL;
static uint32_t dma_buffer_size = 0;

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief ADC初始化
 */
int bsp_adc_hal_init(ADC_HandleTypeDef *hadc)
{
    if (hadc == NULL) {
        return -1;
    }

    hadc_instance = hadc;

    /* 校准ADC */
    /* 注意: STM32F4没有校准功能，此处留空 */

    return 0;
}

/**
 * @brief ADC反初始化
 */
void bsp_adc_hal_deinit(ADC_HandleTypeDef *hadc)
{
    if (hadc != NULL) {
        HAL_ADC_DeInit(hadc);
    }
    hadc_instance = NULL;
}

/**
 * @brief 单次读取
 */
uint16_t bsp_adc_hal_read(ADC_HandleTypeDef *hadc)
{
    uint16_t value = 0;

    if (hadc == NULL) {
        return 0;
    }

    HAL_ADC_Start(hadc);
    if (HAL_ADC_PollForConversion(hadc, 100) == HAL_OK) {
        value = HAL_ADC_GetValue(hadc);
    }
    HAL_ADC_Stop(hadc);

    return value;
}

/**
 * @brief 多次采样取平均
 */
uint16_t bsp_adc_hal_read_average(ADC_HandleTypeDef *hadc, uint8_t times)
{
    uint32_t sum = 0;
    uint8_t i;

    if (hadc == NULL || times == 0) {
        return 0;
    }

    for (i = 0; i < times; i++) {
        sum += bsp_adc_hal_read(hadc);
    }

    return (uint16_t)(sum / times);
}

/**
 * @brief 读取电压值 (mV)
 */
uint32_t bsp_adc_hal_read_voltage(ADC_HandleTypeDef *hadc)
{
    uint16_t raw = bsp_adc_hal_read(hadc);
    /* 3300mV / 4096 = 0.806mV per LSB */
    return (uint32_t)raw * 3300 / 4096;
}

/**
 * @brief ADC值转电压
 */
uint32_t bsp_adc_hal_to_voltage(uint16_t raw_value)
{
    return (uint32_t)raw_value * 3300 / 4096;
}

/**
 * @brief 启动DMA采集
 */
int bsp_adc_hal_start_dma(ADC_HandleTypeDef *hadc, uint16_t *buffer,
                          uint32_t length, adc_hal_callback_t callback)
{
    if (hadc == NULL || buffer == NULL || length == 0) {
        return -1;
    }

    dma_buffer = buffer;
    dma_buffer_size = length;
    dma_callback = callback;

    if (HAL_ADC_Start_DMA(hadc, (uint32_t *)buffer, length) != HAL_OK) {
        return -1;
    }

    return 0;
}

/**
 * @brief 停止DMA采集
 */
void bsp_adc_hal_stop_dma(ADC_HandleTypeDef *hadc)
{
    if (hadc != NULL) {
        HAL_ADC_Stop_DMA(hadc);
    }
    dma_callback = NULL;
}

/**
 * @brief 读取内部温度传感器
 */
int16_t bsp_adc_hal_read_temperature(ADC_HandleTypeDef *hadc)
{
    uint16_t raw;
    int32_t temp;

    if (hadc == NULL) {
        return 0;
    }

    /* 需要配置ADC通道为内部温度传感器通道 */
    raw = bsp_adc_hal_read(hadc);

    /* 温度计算公式 (参考STM32F4参考手册) */
    /* Temperature = ((V25 - Vsense) / Avg_Slope) + 25 */
    /* V25 = 0.76V, Avg_Slope = 2.5mV/°C */
    temp = ((int32_t)((0.76f * 4096 / 3.3f) - raw) * 1000 / 25) + 250;

    return (int16_t)temp;
}

/**
 * @brief DMA传输完成回调 (需要在HAL回调中调用)
 */
void bsp_adc_hal_dma_callback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    if (dma_callback != NULL && dma_buffer != NULL) {
        dma_callback(dma_buffer, dma_buffer_size);
    }
}

#endif /* USE_HAL_DRIVER */
