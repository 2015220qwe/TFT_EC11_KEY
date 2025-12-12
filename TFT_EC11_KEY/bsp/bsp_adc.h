/**
 * @file bsp_adc.h
 * @brief ADC驱动模块 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 硬件平台: STM32F407VGT6
 * @note 功能特性:
 *       - 支持单通道/多通道ADC采样
 *       - 支持DMA连续采样模式
 *       - 支持软件触发/定时器触发
 *       - 内置电压换算功能
 *       - 支持内部温度传感器和VREFINT
 *
 * @note 默认引脚配置:
 *       ADC1_CH0  -> PA0 (与EC11冲突，可选用其他通道)
 *       ADC1_CH1  -> PA1
 *       ADC1_CH4  -> PA4 (推荐使用)
 *       ADC1_CH5  -> PA5
 *       ADC1_CH6  -> PA6
 *       ADC1_CH7  -> PA7
 *       ADC1_CH8  -> PB0
 *       ADC1_CH9  -> PB1
 */

#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* ADC配置 */
#define BSP_ADC_INSTANCE            ADC1
#define BSP_ADC_CLK                 RCC_APB2Periph_ADC1

/* DMA配置 */
#define BSP_ADC_DMA_STREAM          DMA2_Stream0
#define BSP_ADC_DMA_CHANNEL         DMA_Channel_0
#define BSP_ADC_DMA_CLK             RCC_AHB1Periph_DMA2
#define BSP_ADC_DMA_IRQn            DMA2_Stream0_IRQn
#define BSP_ADC_DMA_IRQHandler      DMA2_Stream0_IRQHandler

/* 最大支持的ADC通道数 */
#define BSP_ADC_MAX_CHANNELS        8

/* ADC参考电压 (mV) */
#define BSP_ADC_VREF_MV             3300

/* ADC分辨率 */
#define BSP_ADC_RESOLUTION          4096

/* 波形缓冲区大小 */
#define BSP_ADC_WAVEFORM_BUFFER_SIZE    256

/* 默认采样通道 (PA4) */
#define BSP_ADC_DEFAULT_CHANNEL     ADC_Channel_4
#define BSP_ADC_DEFAULT_GPIO_PORT   GPIOA
#define BSP_ADC_DEFAULT_GPIO_PIN    GPIO_Pin_4
#define BSP_ADC_DEFAULT_GPIO_CLK    RCC_AHB1Periph_GPIOA

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief ADC通道配置结构体
 */
typedef struct {
    uint8_t channel;            /**< ADC通道号 (ADC_Channel_0 ~ ADC_Channel_15) */
    GPIO_TypeDef* gpio_port;    /**< GPIO端口 */
    uint16_t gpio_pin;          /**< GPIO引脚 */
    uint32_t gpio_clk;          /**< GPIO时钟 */
    uint8_t sample_time;        /**< 采样时间 (ADC_SampleTime_xxx) */
} adc_channel_config_t;

/**
 * @brief ADC工作模式
 */
typedef enum {
    ADC_MODE_SINGLE,            /**< 单次转换模式 */
    ADC_MODE_CONTINUOUS,        /**< 连续转换模式 */
    ADC_MODE_SCAN,              /**< 扫描模式(多通道) */
    ADC_MODE_DMA                /**< DMA模式(连续采样) */
} adc_mode_t;

/**
 * @brief ADC采样完成回调函数类型
 */
typedef void (*adc_complete_callback_t)(uint16_t *data, uint32_t len);

/**
 * @brief ADC状态结构体
 */
typedef struct {
    uint8_t is_initialized;     /**< 初始化标志 */
    uint8_t is_running;         /**< 运行状态 */
    adc_mode_t mode;            /**< 当前工作模式 */
    uint8_t channel_count;      /**< 配置的通道数 */
    uint16_t last_value;        /**< 最后一次采样值 */
    uint32_t last_voltage_mv;   /**< 最后一次电压值(mV) */
} adc_state_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief ADC初始化 - 单通道模式
 * @param channel ADC通道配置
 * @retval 0:成功 -1:失败
 */
int bsp_adc_init(const adc_channel_config_t *channel);

/**
 * @brief ADC初始化 - 多通道扫描模式
 * @param channels 通道配置数组
 * @param count 通道数量
 * @retval 0:成功 -1:失败
 */
int bsp_adc_init_multi(const adc_channel_config_t *channels, uint8_t count);

/**
 * @brief ADC初始化 - DMA连续采样模式
 * @param channel ADC通道配置
 * @param buffer 数据缓冲区
 * @param buffer_size 缓冲区大小
 * @param callback 采样完成回调
 * @retval 0:成功 -1:失败
 */
int bsp_adc_init_dma(const adc_channel_config_t *channel,
                     uint16_t *buffer, uint32_t buffer_size,
                     adc_complete_callback_t callback);

/**
 * @brief ADC反初始化
 */
void bsp_adc_deinit(void);

/**
 * @brief 单次ADC采样
 * @retval ADC原始值 (0~4095)
 */
uint16_t bsp_adc_read(void);

/**
 * @brief 读取指定通道的ADC值
 * @param channel ADC通道号
 * @retval ADC原始值 (0~4095)
 */
uint16_t bsp_adc_read_channel(uint8_t channel);

/**
 * @brief 读取多通道ADC值
 * @param data 数据缓冲区
 * @param count 通道数
 * @retval 0:成功 -1:失败
 */
int bsp_adc_read_multi(uint16_t *data, uint8_t count);

/**
 * @brief 获取电压值 (mV)
 * @param raw_value ADC原始值
 * @retval 电压值(mV)
 */
uint32_t bsp_adc_to_voltage(uint16_t raw_value);

/**
 * @brief 直接读取电压值 (mV)
 * @retval 电压值(mV)
 */
uint32_t bsp_adc_read_voltage(void);

/**
 * @brief 读取内部温度传感器温度值
 * @retval 温度值(0.1℃为单位)
 */
int16_t bsp_adc_read_temperature(void);

/**
 * @brief 读取内部参考电压
 * @retval 参考电压(mV)
 */
uint16_t bsp_adc_read_vrefint(void);

/**
 * @brief 启动DMA连续采样
 */
void bsp_adc_dma_start(void);

/**
 * @brief 停止DMA连续采样
 */
void bsp_adc_dma_stop(void);

/**
 * @brief 设置采样率 (仅DMA模式)
 * @param sample_rate 采样率 (Hz)
 */
void bsp_adc_set_sample_rate(uint32_t sample_rate);

/**
 * @brief 获取ADC状态
 * @retval ADC状态结构体指针
 */
const adc_state_t* bsp_adc_get_state(void);

/**
 * @brief ADC多次采样取平均值
 * @param times 采样次数
 * @retval 平均值
 */
uint16_t bsp_adc_read_average(uint8_t times);

/**
 * @brief 获取默认通道配置
 * @retval 默认通道配置
 */
adc_channel_config_t bsp_adc_get_default_config(void);

/*=============================================================================
 *                              默认配置宏
 *============================================================================*/

/**
 * @brief 快速创建ADC通道配置
 */
#define ADC_CHANNEL_CONFIG(ch, port, pin, clk) \
    { .channel = ch, .gpio_port = port, .gpio_pin = pin, \
      .gpio_clk = clk, .sample_time = ADC_SampleTime_84Cycles }

/**
 * @brief 预定义通道配置
 */
#define ADC_CH0_PA0     ADC_CHANNEL_CONFIG(ADC_Channel_0, GPIOA, GPIO_Pin_0, RCC_AHB1Periph_GPIOA)
#define ADC_CH1_PA1     ADC_CHANNEL_CONFIG(ADC_Channel_1, GPIOA, GPIO_Pin_1, RCC_AHB1Periph_GPIOA)
#define ADC_CH4_PA4     ADC_CHANNEL_CONFIG(ADC_Channel_4, GPIOA, GPIO_Pin_4, RCC_AHB1Periph_GPIOA)
#define ADC_CH5_PA5     ADC_CHANNEL_CONFIG(ADC_Channel_5, GPIOA, GPIO_Pin_5, RCC_AHB1Periph_GPIOA)
#define ADC_CH6_PA6     ADC_CHANNEL_CONFIG(ADC_Channel_6, GPIOA, GPIO_Pin_6, RCC_AHB1Periph_GPIOA)
#define ADC_CH7_PA7     ADC_CHANNEL_CONFIG(ADC_Channel_7, GPIOA, GPIO_Pin_7, RCC_AHB1Periph_GPIOA)
#define ADC_CH8_PB0     ADC_CHANNEL_CONFIG(ADC_Channel_8, GPIOB, GPIO_Pin_0, RCC_AHB1Periph_GPIOB)
#define ADC_CH9_PB1     ADC_CHANNEL_CONFIG(ADC_Channel_9, GPIOB, GPIO_Pin_1, RCC_AHB1Periph_GPIOB)

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADC_H */
