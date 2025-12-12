/**
 * @file bsp_timer.c
 * @brief 定时器驱动实现 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_timer.h"
#include <string.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static volatile uint32_t timestamp_ms = 0;
static volatile uint32_t timestamp_us_base = 0;

static timer_callback_t periodic_callback = NULL;

/* 单次定时器 */
typedef struct {
    uint8_t active;
    uint32_t expire_tick;
    timer_callback_t callback;
} oneshot_timer_t;

static oneshot_timer_t oneshot_timers[TIMER_CHANNEL_MAX] = {0};

/* 超时检测 */
#define MAX_TIMEOUT_COUNT   8

typedef struct {
    uint8_t active;
    uint32_t timeout_ms;
    uint32_t last_feed_tick;
    timeout_callback_t callback;
} timeout_entry_t;

static timeout_entry_t timeout_entries[MAX_TIMEOUT_COUNT] = {0};

/* 时间测量 */
static uint32_t measure_start_us = 0;

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief 定时器模块初始化
 */
void bsp_timer_init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 初始化时间戳定时器 TIM6 */
    RCC_APB1PeriphClockCmd(TIMER_TIMESTAMP_CLK, ENABLE);

    /* 1ms中断 */
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;  /* 84MHz / 84 = 1MHz */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIMER_TIMESTAMP_TIM, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIMER_TIMESTAMP_TIM, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIMER_TIMESTAMP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIMER_TIMESTAMP_TIM, ENABLE);

    /* 初始化微秒定时器 TIM7 */
    RCC_APB1PeriphClockCmd(TIMER_US_CLK, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;  /* 1MHz, 1us精度 */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIMER_US_TIM, &TIM_TimeBaseStructure);

    TIM_Cmd(TIMER_US_TIM, ENABLE);

    /* 清空状态 */
    timestamp_ms = 0;
    timestamp_us_base = 0;
    memset(oneshot_timers, 0, sizeof(oneshot_timers));
    memset(timeout_entries, 0, sizeof(timeout_entries));
}

/**
 * @brief 定时器模块反初始化
 */
void bsp_timer_deinit(void)
{
    TIM_Cmd(TIMER_TIMESTAMP_TIM, DISABLE);
    TIM_Cmd(TIMER_US_TIM, DISABLE);
    TIM_DeInit(TIMER_TIMESTAMP_TIM);
    TIM_DeInit(TIMER_US_TIM);
}

/**
 * @brief 获取当前时间戳 (ms)
 */
uint32_t bsp_timer_get_ms(void)
{
    return timestamp_ms;
}

/**
 * @brief 获取当前时间戳 (us)
 */
uint32_t bsp_timer_get_us(void)
{
    uint32_t ms = timestamp_ms;
    uint16_t us = TIM_GetCounter(TIMER_US_TIM);
    return ms * 1000 + us;
}

/**
 * @brief 时间戳中断处理
 */
void bsp_timer_timestamp_isr(void)
{
    if (TIM_GetITStatus(TIMER_TIMESTAMP_TIM, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIMER_TIMESTAMP_TIM, TIM_IT_Update);
        timestamp_ms++;
    }
}

/**
 * @brief 微秒级阻塞延时
 */
void bsp_timer_delay_us(uint32_t us)
{
    uint32_t start = TIM_GetCounter(TIMER_US_TIM);
    uint32_t target;

    while (us > 0) {
        uint32_t delay = (us > 60000) ? 60000 : us;
        target = (start + delay) % 65536;

        if (target > start) {
            while (TIM_GetCounter(TIMER_US_TIM) < target);
        } else {
            while (TIM_GetCounter(TIMER_US_TIM) >= start);
            while (TIM_GetCounter(TIMER_US_TIM) < target);
        }

        us -= delay;
        start = target;
    }
}

/**
 * @brief 毫秒级阻塞延时
 */
void bsp_timer_delay_ms(uint32_t ms)
{
    uint32_t start = timestamp_ms;
    while ((timestamp_ms - start) < ms);
}

/**
 * @brief 启动周期定时器
 */
int bsp_timer_start_periodic(uint32_t period_us, timer_callback_t callback)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uint16_t prescaler, period;

    if (callback == NULL || period_us == 0) {
        return -1;
    }

    periodic_callback = callback;

    RCC_APB1PeriphClockCmd(TIMER_PERIODIC_CLK, ENABLE);

    /* 计算分频和周期 */
    if (period_us < 65536) {
        prescaler = 84 - 1;     /* 1MHz */
        period = period_us - 1;
    } else {
        prescaler = 8400 - 1;   /* 10kHz */
        period = period_us / 100 - 1;
    }

    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIMER_PERIODIC_TIM, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIMER_PERIODIC_TIM, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIMER_PERIODIC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIMER_PERIODIC_TIM, ENABLE);

    return 0;
}

/**
 * @brief 停止周期定时器
 */
void bsp_timer_stop_periodic(void)
{
    TIM_Cmd(TIMER_PERIODIC_TIM, DISABLE);
    TIM_ITConfig(TIMER_PERIODIC_TIM, TIM_IT_Update, DISABLE);
    periodic_callback = NULL;
}

/**
 * @brief 设置周期定时器周期
 */
void bsp_timer_set_periodic_period(uint32_t period_us)
{
    uint16_t period;

    if (period_us < 65536) {
        period = period_us - 1;
    } else {
        period = period_us / 100 - 1;
    }

    TIM_SetAutoreload(TIMER_PERIODIC_TIM, period);
}

/**
 * @brief 周期定时器中断处理
 */
void bsp_timer_periodic_isr(void)
{
    if (TIM_GetITStatus(TIMER_PERIODIC_TIM, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIMER_PERIODIC_TIM, TIM_IT_Update);

        if (periodic_callback != NULL) {
            periodic_callback();
        }
    }
}

/**
 * @brief 启动单次定时器
 */
int bsp_timer_start_oneshot(timer_channel_t channel, uint32_t delay_ms, timer_callback_t callback)
{
    if (channel >= TIMER_CHANNEL_MAX || callback == NULL) {
        return -1;
    }

    oneshot_timers[channel].active = 1;
    oneshot_timers[channel].expire_tick = timestamp_ms + delay_ms;
    oneshot_timers[channel].callback = callback;

    return 0;
}

/**
 * @brief 取消单次定时器
 */
void bsp_timer_cancel_oneshot(timer_channel_t channel)
{
    if (channel >= TIMER_CHANNEL_MAX) {
        return;
    }

    oneshot_timers[channel].active = 0;
}

/**
 * @brief 单次定时器检查
 */
void bsp_timer_oneshot_process(void)
{
    uint8_t i;
    uint32_t current_tick = timestamp_ms;

    for (i = 0; i < TIMER_CHANNEL_MAX; i++) {
        if (oneshot_timers[i].active) {
            if ((int32_t)(current_tick - oneshot_timers[i].expire_tick) >= 0) {
                oneshot_timers[i].active = 0;
                if (oneshot_timers[i].callback != NULL) {
                    oneshot_timers[i].callback();
                }
            }
        }
    }
}

/**
 * @brief 启动超时检测
 */
int bsp_timer_timeout_start(uint8_t id, uint32_t timeout_ms, timeout_callback_t callback)
{
    if (id >= MAX_TIMEOUT_COUNT || callback == NULL) {
        return -1;
    }

    timeout_entries[id].active = 1;
    timeout_entries[id].timeout_ms = timeout_ms;
    timeout_entries[id].last_feed_tick = timestamp_ms;
    timeout_entries[id].callback = callback;

    return 0;
}

/**
 * @brief 喂狗
 */
void bsp_timer_timeout_feed(uint8_t id)
{
    if (id >= MAX_TIMEOUT_COUNT) {
        return;
    }

    timeout_entries[id].last_feed_tick = timestamp_ms;
}

/**
 * @brief 停止超时检测
 */
void bsp_timer_timeout_stop(uint8_t id)
{
    if (id >= MAX_TIMEOUT_COUNT) {
        return;
    }

    timeout_entries[id].active = 0;
}

/**
 * @brief 超时检测处理
 */
void bsp_timer_timeout_process(void)
{
    uint8_t i;
    uint32_t current_tick = timestamp_ms;

    for (i = 0; i < MAX_TIMEOUT_COUNT; i++) {
        if (timeout_entries[i].active) {
            if ((current_tick - timeout_entries[i].last_feed_tick) >= timeout_entries[i].timeout_ms) {
                if (timeout_entries[i].callback != NULL) {
                    timeout_entries[i].callback(i);
                }
                /* 重置 */
                timeout_entries[i].last_feed_tick = current_tick;
            }
        }
    }
}

/**
 * @brief 开始测量
 */
void bsp_timer_measure_start(void)
{
    measure_start_us = bsp_timer_get_us();
}

/**
 * @brief 结束测量
 */
uint32_t bsp_timer_measure_stop(void)
{
    return bsp_timer_get_us() - measure_start_us;
}

/**
 * @brief 获取系统运行时间 (秒)
 */
uint32_t bsp_timer_get_uptime_sec(void)
{
    return timestamp_ms / 1000;
}

/**
 * @brief 获取格式化的运行时间
 */
void bsp_timer_get_uptime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds)
{
    uint32_t total_sec = timestamp_ms / 1000;

    if (hours) *hours = total_sec / 3600;
    if (minutes) *minutes = (total_sec % 3600) / 60;
    if (seconds) *seconds = total_sec % 60;
}

/*=============================================================================
 *                              中断服务函数
 *============================================================================*/

void TIM6_DAC_IRQHandler(void)
{
    bsp_timer_timestamp_isr();
}

void TIM2_IRQHandler(void)
{
    bsp_timer_periodic_isr();
}
