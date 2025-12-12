/**
 * @file bsp_adc.c
 * @brief ADC驱动模块实现 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_adc.h"

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static adc_state_t adc_state = {0};
static adc_complete_callback_t dma_callback = NULL;
static uint16_t *dma_buffer = NULL;
static uint32_t dma_buffer_size = 0;
static adc_channel_config_t current_channels[BSP_ADC_MAX_CHANNELS];

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static void adc_gpio_init(const adc_channel_config_t *channel);
static void adc_gpio_init_multi(const adc_channel_config_t *channels, uint8_t count);
static void adc_dma_init(uint16_t *buffer, uint32_t size);

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief 获取默认通道配置
 */
adc_channel_config_t bsp_adc_get_default_config(void)
{
    adc_channel_config_t config = {
        .channel = BSP_ADC_DEFAULT_CHANNEL,
        .gpio_port = BSP_ADC_DEFAULT_GPIO_PORT,
        .gpio_pin = BSP_ADC_DEFAULT_GPIO_PIN,
        .gpio_clk = BSP_ADC_DEFAULT_GPIO_CLK,
        .sample_time = ADC_SampleTime_84Cycles
    };
    return config;
}

/**
 * @brief ADC初始化 - 单通道模式
 */
int bsp_adc_init(const adc_channel_config_t *channel)
{
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    if (channel == NULL) {
        return -1;
    }

    /* 保存通道配置 */
    current_channels[0] = *channel;
    adc_state.channel_count = 1;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(BSP_ADC_CLK, ENABLE);

    /* 初始化GPIO */
    adc_gpio_init(channel);

    /* ADC通用配置 */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC配置 */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(BSP_ADC_INSTANCE, &ADC_InitStructure);

    /* 配置ADC通道 */
    ADC_RegularChannelConfig(BSP_ADC_INSTANCE, channel->channel, 1, channel->sample_time);

    /* 使能ADC */
    ADC_Cmd(BSP_ADC_INSTANCE, ENABLE);

    adc_state.is_initialized = 1;
    adc_state.mode = ADC_MODE_SINGLE;

    return 0;
}

/**
 * @brief ADC初始化 - 多通道扫描模式
 */
int bsp_adc_init_multi(const adc_channel_config_t *channels, uint8_t count)
{
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    uint8_t i;

    if (channels == NULL || count == 0 || count > BSP_ADC_MAX_CHANNELS) {
        return -1;
    }

    /* 保存通道配置 */
    for (i = 0; i < count; i++) {
        current_channels[i] = channels[i];
    }
    adc_state.channel_count = count;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(BSP_ADC_CLK, ENABLE);

    /* 初始化GPIO */
    adc_gpio_init_multi(channels, count);

    /* ADC通用配置 */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC配置 - 扫描模式 */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = count;
    ADC_Init(BSP_ADC_INSTANCE, &ADC_InitStructure);

    /* 配置各通道 */
    for (i = 0; i < count; i++) {
        ADC_RegularChannelConfig(BSP_ADC_INSTANCE, channels[i].channel, i + 1, channels[i].sample_time);
    }

    /* 使能ADC */
    ADC_Cmd(BSP_ADC_INSTANCE, ENABLE);

    adc_state.is_initialized = 1;
    adc_state.mode = ADC_MODE_SCAN;

    return 0;
}

/**
 * @brief ADC初始化 - DMA连续采样模式
 */
int bsp_adc_init_dma(const adc_channel_config_t *channel,
                     uint16_t *buffer, uint32_t buffer_size,
                     adc_complete_callback_t callback)
{
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    if (channel == NULL || buffer == NULL || buffer_size == 0) {
        return -1;
    }

    /* 保存DMA配置 */
    dma_buffer = buffer;
    dma_buffer_size = buffer_size;
    dma_callback = callback;

    /* 保存通道配置 */
    current_channels[0] = *channel;
    adc_state.channel_count = 1;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(BSP_ADC_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(BSP_ADC_DMA_CLK, ENABLE);

    /* 初始化GPIO */
    adc_gpio_init(channel);

    /* 初始化DMA */
    adc_dma_init(buffer, buffer_size);

    /* ADC通用配置 */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC配置 - 连续转换+DMA */
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(BSP_ADC_INSTANCE, &ADC_InitStructure);

    /* 配置ADC通道 */
    ADC_RegularChannelConfig(BSP_ADC_INSTANCE, channel->channel, 1, channel->sample_time);

    /* 使能ADC DMA */
    ADC_DMACmd(BSP_ADC_INSTANCE, ENABLE);
    ADC_DMARequestAfterLastTransferCmd(BSP_ADC_INSTANCE, ENABLE);

    /* 使能ADC */
    ADC_Cmd(BSP_ADC_INSTANCE, ENABLE);

    adc_state.is_initialized = 1;
    adc_state.mode = ADC_MODE_DMA;

    return 0;
}

/**
 * @brief ADC反初始化
 */
void bsp_adc_deinit(void)
{
    /* 停止DMA */
    if (adc_state.mode == ADC_MODE_DMA) {
        bsp_adc_dma_stop();
        DMA_DeInit(BSP_ADC_DMA_STREAM);
    }

    /* 关闭ADC */
    ADC_Cmd(BSP_ADC_INSTANCE, DISABLE);
    ADC_DeInit();

    /* 清除状态 */
    adc_state.is_initialized = 0;
    adc_state.is_running = 0;
    adc_state.channel_count = 0;
    dma_callback = NULL;
    dma_buffer = NULL;
    dma_buffer_size = 0;
}

/**
 * @brief 单次ADC采样
 */
uint16_t bsp_adc_read(void)
{
    if (!adc_state.is_initialized) {
        return 0;
    }

    /* 启动转换 */
    ADC_SoftwareStartConv(BSP_ADC_INSTANCE);

    /* 等待转换完成 */
    while (ADC_GetFlagStatus(BSP_ADC_INSTANCE, ADC_FLAG_EOC) == RESET);

    /* 读取结果 */
    adc_state.last_value = ADC_GetConversionValue(BSP_ADC_INSTANCE);
    adc_state.last_voltage_mv = bsp_adc_to_voltage(adc_state.last_value);

    return adc_state.last_value;
}

/**
 * @brief 读取指定通道的ADC值
 */
uint16_t bsp_adc_read_channel(uint8_t channel)
{
    if (!adc_state.is_initialized) {
        return 0;
    }

    /* 配置通道 */
    ADC_RegularChannelConfig(BSP_ADC_INSTANCE, channel, 1, ADC_SampleTime_84Cycles);

    /* 启动转换 */
    ADC_SoftwareStartConv(BSP_ADC_INSTANCE);

    /* 等待转换完成 */
    while (ADC_GetFlagStatus(BSP_ADC_INSTANCE, ADC_FLAG_EOC) == RESET);

    return ADC_GetConversionValue(BSP_ADC_INSTANCE);
}

/**
 * @brief 读取多通道ADC值
 */
int bsp_adc_read_multi(uint16_t *data, uint8_t count)
{
    uint8_t i;

    if (!adc_state.is_initialized || data == NULL || count > adc_state.channel_count) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        /* 启动转换 */
        ADC_SoftwareStartConv(BSP_ADC_INSTANCE);

        /* 等待转换完成 */
        while (ADC_GetFlagStatus(BSP_ADC_INSTANCE, ADC_FLAG_EOC) == RESET);

        data[i] = ADC_GetConversionValue(BSP_ADC_INSTANCE);
    }

    return 0;
}

/**
 * @brief ADC值转换为电压值 (mV)
 */
uint32_t bsp_adc_to_voltage(uint16_t raw_value)
{
    return (uint32_t)raw_value * BSP_ADC_VREF_MV / BSP_ADC_RESOLUTION;
}

/**
 * @brief 直接读取电压值 (mV)
 */
uint32_t bsp_adc_read_voltage(void)
{
    uint16_t raw = bsp_adc_read();
    return bsp_adc_to_voltage(raw);
}

/**
 * @brief 读取内部温度传感器温度值
 * @note 公式: Temperature = ((V_SENSE - V25) / Avg_Slope) + 25
 *       V25 = 0.76V, Avg_Slope = 2.5mV/℃
 */
int16_t bsp_adc_read_temperature(void)
{
    uint16_t adc_value;
    int32_t temperature;

    /* 使能温度传感器 */
    ADC_TempSensorVrefintCmd(ENABLE);

    /* 配置温度传感器通道 (Channel 16) */
    ADC_RegularChannelConfig(BSP_ADC_INSTANCE, ADC_Channel_TempSensor, 1, ADC_SampleTime_480Cycles);

    /* 启动转换 */
    ADC_SoftwareStartConv(BSP_ADC_INSTANCE);

    /* 等待转换完成 */
    while (ADC_GetFlagStatus(BSP_ADC_INSTANCE, ADC_FLAG_EOC) == RESET);

    adc_value = ADC_GetConversionValue(BSP_ADC_INSTANCE);

    /* 计算温度 (单位: 0.1℃) */
    /* V_SENSE = adc_value * VREF / 4096 */
    /* Temperature = ((V_SENSE - 0.76) / 0.0025) + 25 */
    temperature = ((int32_t)adc_value * BSP_ADC_VREF_MV / BSP_ADC_RESOLUTION - 760) * 10 / 25 + 250;

    /* 恢复原通道配置 */
    if (adc_state.channel_count > 0) {
        ADC_RegularChannelConfig(BSP_ADC_INSTANCE, current_channels[0].channel, 1, current_channels[0].sample_time);
    }

    return (int16_t)temperature;
}

/**
 * @brief 读取内部参考电压
 */
uint16_t bsp_adc_read_vrefint(void)
{
    uint16_t adc_value;

    /* 使能参考电压 */
    ADC_TempSensorVrefintCmd(ENABLE);

    /* 配置VREFINT通道 (Channel 17) */
    ADC_RegularChannelConfig(BSP_ADC_INSTANCE, ADC_Channel_Vrefint, 1, ADC_SampleTime_480Cycles);

    /* 启动转换 */
    ADC_SoftwareStartConv(BSP_ADC_INSTANCE);

    /* 等待转换完成 */
    while (ADC_GetFlagStatus(BSP_ADC_INSTANCE, ADC_FLAG_EOC) == RESET);

    adc_value = ADC_GetConversionValue(BSP_ADC_INSTANCE);

    /* 恢复原通道配置 */
    if (adc_state.channel_count > 0) {
        ADC_RegularChannelConfig(BSP_ADC_INSTANCE, current_channels[0].channel, 1, current_channels[0].sample_time);
    }

    /* 计算实际VREFINT电压 (标准值约1.2V) */
    return (uint16_t)((uint32_t)adc_value * BSP_ADC_VREF_MV / BSP_ADC_RESOLUTION);
}

/**
 * @brief 启动DMA连续采样
 */
void bsp_adc_dma_start(void)
{
    if (!adc_state.is_initialized || adc_state.mode != ADC_MODE_DMA) {
        return;
    }

    /* 使能DMA流 */
    DMA_Cmd(BSP_ADC_DMA_STREAM, ENABLE);

    /* 启动ADC转换 */
    ADC_SoftwareStartConv(BSP_ADC_INSTANCE);

    adc_state.is_running = 1;
}

/**
 * @brief 停止DMA连续采样
 */
void bsp_adc_dma_stop(void)
{
    if (!adc_state.is_initialized || adc_state.mode != ADC_MODE_DMA) {
        return;
    }

    /* 停止DMA */
    DMA_Cmd(BSP_ADC_DMA_STREAM, DISABLE);

    adc_state.is_running = 0;
}

/**
 * @brief 获取ADC状态
 */
const adc_state_t* bsp_adc_get_state(void)
{
    return &adc_state;
}

/**
 * @brief ADC多次采样取平均值
 */
uint16_t bsp_adc_read_average(uint8_t times)
{
    uint32_t sum = 0;
    uint8_t i;

    if (times == 0) times = 1;
    if (times > 64) times = 64;

    for (i = 0; i < times; i++) {
        sum += bsp_adc_read();
    }

    return (uint16_t)(sum / times);
}

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

/**
 * @brief ADC GPIO初始化
 */
static void adc_gpio_init(const adc_channel_config_t *channel)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIO时钟 */
    RCC_AHB1PeriphClockCmd(channel->gpio_clk, ENABLE);

    /* 配置为模拟输入 */
    GPIO_InitStructure.GPIO_Pin = channel->gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(channel->gpio_port, &GPIO_InitStructure);
}

/**
 * @brief ADC多通道GPIO初始化
 */
static void adc_gpio_init_multi(const adc_channel_config_t *channels, uint8_t count)
{
    uint8_t i;
    for (i = 0; i < count; i++) {
        adc_gpio_init(&channels[i]);
    }
}

/**
 * @brief ADC DMA初始化
 */
static void adc_dma_init(uint16_t *buffer, uint32_t size)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* DMA配置 */
    DMA_DeInit(BSP_ADC_DMA_STREAM);
    DMA_InitStructure.DMA_Channel = BSP_ADC_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&BSP_ADC_INSTANCE->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = size;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(BSP_ADC_DMA_STREAM, &DMA_InitStructure);

    /* 使能DMA传输完成中断 */
    DMA_ITConfig(BSP_ADC_DMA_STREAM, DMA_IT_TC, ENABLE);

    /* 配置NVIC */
    NVIC_InitStructure.NVIC_IRQChannel = BSP_ADC_DMA_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*=============================================================================
 *                              中断服务函数
 *============================================================================*/

/**
 * @brief DMA传输完成中断处理
 */
void BSP_ADC_DMA_IRQHandler(void)
{
    if (DMA_GetITStatus(BSP_ADC_DMA_STREAM, DMA_IT_TCIF0) != RESET) {
        DMA_ClearITPendingBit(BSP_ADC_DMA_STREAM, DMA_IT_TCIF0);

        /* 调用回调函数 */
        if (dma_callback != NULL) {
            dma_callback(dma_buffer, dma_buffer_size);
        }
    }
}
