/**
 * @file bsp_pwm.c
 * @brief PWM输出驱动实现 - 标准库版本
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_pwm.h"
#include <string.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

typedef struct {
    uint8_t is_used;
    pwm_config_t config;
    TIM_TypeDef *timer;
    uint16_t current_duty;
    /* 呼吸灯状态 */
    uint8_t breath_active;
    uint8_t breath_direction; /* 0:增加 1:减少 */
    pwm_breath_param_t breath_param;
    uint32_t breath_last_tick;
} pwm_channel_info_t;

static pwm_channel_info_t pwm_channels[PWM_MAX_CHANNELS] = {0};

/* 延时函数 */
extern uint32_t scheduler_get_tick(void);

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static TIM_TypeDef* get_timer_instance(pwm_timer_t timer);
static uint32_t get_timer_clk(pwm_timer_t timer);
static void enable_timer_clk(pwm_timer_t timer);
static void config_timer_channel(TIM_TypeDef *tim, pwm_ch_t ch, uint16_t pulse);

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief PWM初始化
 */
pwm_channel_t bsp_pwm_init(const pwm_config_t *config)
{
    uint8_t i;
    pwm_channel_t channel = 0xFF;
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TypeDef *tim;
    uint32_t timer_clk;
    uint16_t prescaler, period;

    if (config == NULL) {
        return 0xFF;
    }

    /* 查找空闲通道 */
    for (i = 0; i < PWM_MAX_CHANNELS; i++) {
        if (!pwm_channels[i].is_used) {
            channel = i;
            break;
        }
    }

    if (channel == 0xFF) {
        return 0xFF;
    }

    /* 获取定时器实例 */
    tim = get_timer_instance(config->timer);
    if (tim == NULL) {
        return 0xFF;
    }

    /* 使能时钟 */
    enable_timer_clk(config->timer);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);

    /* 配置GPIO */
    GPIO_InitStructure.GPIO_Pin = config->gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(config->gpio_port, &GPIO_InitStructure);

    GPIO_PinAFConfig(config->gpio_port, config->gpio_pinsrc, config->gpio_af);

    /* 计算预分频和周期 */
    timer_clk = get_timer_clk(config->timer);
    prescaler = (timer_clk / (config->frequency * config->resolution)) - 1;
    period = config->resolution - 1;

    /* 配置定时器 */
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(tim, &TIM_TimeBaseStructure);

    /* 配置PWM通道 */
    config_timer_channel(tim, config->channel, 0);

    /* 使能定时器 */
    TIM_Cmd(tim, ENABLE);

    /* 保存配置 */
    pwm_channels[channel].is_used = 1;
    pwm_channels[channel].config = *config;
    pwm_channels[channel].timer = tim;
    pwm_channels[channel].current_duty = 0;
    pwm_channels[channel].breath_active = 0;

    return channel;
}

/**
 * @brief PWM反初始化
 */
void bsp_pwm_deinit(pwm_channel_t channel)
{
    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    bsp_pwm_stop(channel);
    pwm_channels[channel].is_used = 0;
}

/**
 * @brief 设置占空比
 */
void bsp_pwm_set_duty(pwm_channel_t channel, uint16_t duty)
{
    TIM_TypeDef *tim;
    pwm_ch_t ch;

    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    tim = pwm_channels[channel].timer;
    ch = pwm_channels[channel].config.channel;

    /* 限制范围 */
    if (duty > pwm_channels[channel].config.resolution) {
        duty = pwm_channels[channel].config.resolution;
    }

    /* 设置比较值 */
    switch (ch) {
    case PWM_CH_1: TIM_SetCompare1(tim, duty); break;
    case PWM_CH_2: TIM_SetCompare2(tim, duty); break;
    case PWM_CH_3: TIM_SetCompare3(tim, duty); break;
    case PWM_CH_4: TIM_SetCompare4(tim, duty); break;
    }

    pwm_channels[channel].current_duty = duty;
}

/**
 * @brief 设置占空比 (百分比)
 */
void bsp_pwm_set_duty_percent(pwm_channel_t channel, float percent)
{
    uint16_t duty;

    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    duty = (uint16_t)(percent * pwm_channels[channel].config.resolution / 100.0f);
    bsp_pwm_set_duty(channel, duty);
}

/**
 * @brief 获取占空比
 */
uint16_t bsp_pwm_get_duty(pwm_channel_t channel)
{
    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return 0;
    }

    return pwm_channels[channel].current_duty;
}

/**
 * @brief 设置频率
 */
int bsp_pwm_set_frequency(pwm_channel_t channel, uint32_t freq)
{
    TIM_TypeDef *tim;
    uint32_t timer_clk;
    uint16_t prescaler, period;
    uint16_t resolution;

    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return -1;
    }

    tim = pwm_channels[channel].timer;
    resolution = pwm_channels[channel].config.resolution;
    timer_clk = get_timer_clk(pwm_channels[channel].config.timer);

    prescaler = (timer_clk / (freq * resolution)) - 1;
    period = resolution - 1;

    TIM_SetAutoreload(tim, period);
    TIM_PrescalerConfig(tim, prescaler, TIM_PSCReloadMode_Immediate);

    pwm_channels[channel].config.frequency = freq;

    return 0;
}

/**
 * @brief 获取频率
 */
uint32_t bsp_pwm_get_frequency(pwm_channel_t channel)
{
    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return 0;
    }

    return pwm_channels[channel].config.frequency;
}

/**
 * @brief 启动PWM输出
 */
void bsp_pwm_start(pwm_channel_t channel)
{
    TIM_TypeDef *tim;
    pwm_ch_t ch;

    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    tim = pwm_channels[channel].timer;
    ch = pwm_channels[channel].config.channel;

    switch (ch) {
    case PWM_CH_1: TIM_CCxCmd(tim, TIM_Channel_1, TIM_CCx_Enable); break;
    case PWM_CH_2: TIM_CCxCmd(tim, TIM_Channel_2, TIM_CCx_Enable); break;
    case PWM_CH_3: TIM_CCxCmd(tim, TIM_Channel_3, TIM_CCx_Enable); break;
    case PWM_CH_4: TIM_CCxCmd(tim, TIM_Channel_4, TIM_CCx_Enable); break;
    }
}

/**
 * @brief 停止PWM输出
 */
void bsp_pwm_stop(pwm_channel_t channel)
{
    TIM_TypeDef *tim;
    pwm_ch_t ch;

    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    tim = pwm_channels[channel].timer;
    ch = pwm_channels[channel].config.channel;

    switch (ch) {
    case PWM_CH_1: TIM_CCxCmd(tim, TIM_Channel_1, TIM_CCx_Disable); break;
    case PWM_CH_2: TIM_CCxCmd(tim, TIM_Channel_2, TIM_CCx_Disable); break;
    case PWM_CH_3: TIM_CCxCmd(tim, TIM_Channel_3, TIM_CCx_Disable); break;
    case PWM_CH_4: TIM_CCxCmd(tim, TIM_Channel_4, TIM_CCx_Disable); break;
    }
}

/**
 * @brief 启动呼吸灯效果
 */
void bsp_pwm_breath_start(pwm_channel_t channel, const pwm_breath_param_t *param)
{
    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    if (param != NULL) {
        pwm_channels[channel].breath_param = *param;
    } else {
        /* 默认参数 */
        pwm_channels[channel].breath_param.min_duty = 0;
        pwm_channels[channel].breath_param.max_duty = pwm_channels[channel].config.resolution;
        pwm_channels[channel].breath_param.step = pwm_channels[channel].config.resolution / 100;
        pwm_channels[channel].breath_param.interval_ms = 20;
    }

    pwm_channels[channel].breath_active = 1;
    pwm_channels[channel].breath_direction = 0;
    pwm_channels[channel].breath_last_tick = 0;

    bsp_pwm_set_duty(channel, pwm_channels[channel].breath_param.min_duty);
    bsp_pwm_start(channel);
}

/**
 * @brief 停止呼吸灯效果
 */
void bsp_pwm_breath_stop(pwm_channel_t channel)
{
    if (channel >= PWM_MAX_CHANNELS || !pwm_channels[channel].is_used) {
        return;
    }

    pwm_channels[channel].breath_active = 0;
}

/**
 * @brief 呼吸灯更新
 */
void bsp_pwm_breath_update(void)
{
    uint8_t i;
    uint32_t current_tick = scheduler_get_tick();
    pwm_breath_param_t *param;
    uint16_t duty;

    for (i = 0; i < PWM_MAX_CHANNELS; i++) {
        if (pwm_channels[i].is_used && pwm_channels[i].breath_active) {
            param = &pwm_channels[i].breath_param;

            if (current_tick - pwm_channels[i].breath_last_tick >= param->interval_ms) {
                pwm_channels[i].breath_last_tick = current_tick;

                duty = pwm_channels[i].current_duty;

                if (pwm_channels[i].breath_direction == 0) {
                    /* 增加 */
                    if (duty + param->step >= param->max_duty) {
                        duty = param->max_duty;
                        pwm_channels[i].breath_direction = 1;
                    } else {
                        duty += param->step;
                    }
                } else {
                    /* 减少 */
                    if (duty <= param->min_duty + param->step) {
                        duty = param->min_duty;
                        pwm_channels[i].breath_direction = 0;
                    } else {
                        duty -= param->step;
                    }
                }

                bsp_pwm_set_duty(i, duty);
            }
        }
    }
}

/**
 * @brief 获取预定义配置
 */
pwm_config_t bsp_pwm_get_preset_config(pwm_timer_t timer, pwm_ch_t ch)
{
    pwm_config_t config = {
        .timer = timer,
        .channel = ch,
        .frequency = PWM_DEFAULT_FREQ,
        .resolution = PWM_DEFAULT_RESOLUTION
    };

    /* 根据定时器和通道设置GPIO */
    if (timer == PWM_TIMER_3) {
        if (ch == PWM_CH_1) {
            config.gpio_port = PWM_TIM3_CH1_PORT;
            config.gpio_pin = PWM_TIM3_CH1_PIN;
            config.gpio_pinsrc = PWM_TIM3_CH1_PINSRC;
            config.gpio_af = PWM_TIM3_CH1_AF;
        } else if (ch == PWM_CH_2) {
            config.gpio_port = PWM_TIM3_CH2_PORT;
            config.gpio_pin = PWM_TIM3_CH2_PIN;
            config.gpio_pinsrc = PWM_TIM3_CH2_PINSRC;
            config.gpio_af = PWM_TIM3_CH2_AF;
        }
    } else if (timer == PWM_TIMER_4) {
        if (ch == PWM_CH_1) {
            config.gpio_port = PWM_TIM4_CH1_PORT;
            config.gpio_pin = PWM_TIM4_CH1_PIN;
            config.gpio_pinsrc = PWM_TIM4_CH1_PINSRC;
            config.gpio_af = PWM_TIM4_CH1_AF;
        } else if (ch == PWM_CH_2) {
            config.gpio_port = PWM_TIM4_CH2_PORT;
            config.gpio_pin = PWM_TIM4_CH2_PIN;
            config.gpio_pinsrc = PWM_TIM4_CH2_PINSRC;
            config.gpio_af = PWM_TIM4_CH2_AF;
        }
    }

    return config;
}

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

static TIM_TypeDef* get_timer_instance(pwm_timer_t timer)
{
    switch (timer) {
    case PWM_TIMER_1: return TIM1;
    case PWM_TIMER_2: return TIM2;
    case PWM_TIMER_3: return TIM3;
    case PWM_TIMER_4: return TIM4;
    case PWM_TIMER_5: return TIM5;
    case PWM_TIMER_8: return TIM8;
    default: return NULL;
    }
}

static uint32_t get_timer_clk(pwm_timer_t timer)
{
    /* APB1定时器时钟: 84MHz, APB2定时器时钟: 168MHz */
    switch (timer) {
    case PWM_TIMER_1:
    case PWM_TIMER_8:
        return 168000000; /* APB2 */
    default:
        return 84000000;  /* APB1 */
    }
}

static void enable_timer_clk(pwm_timer_t timer)
{
    switch (timer) {
    case PWM_TIMER_1: RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); break;
    case PWM_TIMER_2: RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); break;
    case PWM_TIMER_3: RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); break;
    case PWM_TIMER_4: RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); break;
    case PWM_TIMER_5: RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); break;
    case PWM_TIMER_8: RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE); break;
    default: break;
    }
}

static void config_timer_channel(TIM_TypeDef *tim, pwm_ch_t ch, uint16_t pulse)
{
    TIM_OCInitTypeDef TIM_OCInitStructure;

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = pulse;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    switch (ch) {
    case PWM_CH_1:
        TIM_OC1Init(tim, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
        break;
    case PWM_CH_2:
        TIM_OC2Init(tim, &TIM_OCInitStructure);
        TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
        break;
    case PWM_CH_3:
        TIM_OC3Init(tim, &TIM_OCInitStructure);
        TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
        break;
    case PWM_CH_4:
        TIM_OC4Init(tim, &TIM_OCInitStructure);
        TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
        break;
    }

    TIM_ARRPreloadConfig(tim, ENABLE);

    /* 高级定时器需要使能MOE */
    if (tim == TIM1 || tim == TIM8) {
        TIM_CtrlPWMOutputs(tim, ENABLE);
    }
}
