/**
  ******************************************************************************
  * @file    menu_animation.c
  * @author  TFT_EC11_KEY Project
  * @brief   菜单动画效果实现
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "menu_animation.h"
#include <math.h>

/* 外部时间戳函数 */
extern uint32_t bsp_ec11_get_tick(void);

/* Private variables ---------------------------------------------------------*/
static menu_animation_t current_anim = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  线性插值
 * @param  progress: 进度 (0.0-1.0)
 * @retval 插值结果
 */
static float easing_linear(float progress)
{
    return progress;
}

/**
 * @brief  加速插值 (Ease In)
 * @param  progress: 进度 (0.0-1.0)
 * @retval 插值结果
 */
static float easing_ease_in(float progress)
{
    return progress * progress;
}

/**
 * @brief  减速插值 (Ease Out)
 * @param  progress: 进度 (0.0-1.0)
 * @retval 插值结果
 */
static float easing_ease_out(float progress)
{
    return 1.0f - (1.0f - progress) * (1.0f - progress);
}

/**
 * @brief  先加速后减速 (Ease In Out)
 * @param  progress: 进度 (0.0-1.0)
 * @retval 插值结果
 */
static float easing_ease_in_out(float progress)
{
    if (progress < 0.5f) {
        return 2.0f * progress * progress;
    } else {
        float temp = -2.0f * progress + 2.0f;
        return 1.0f - temp * temp / 2.0f;
    }
}

/**
 * @brief  弹跳插值
 * @param  progress: 进度 (0.0-1.0)
 * @retval 插值结果
 */
static float easing_bounce(float progress)
{
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (progress < 1.0f / d1) {
        return n1 * progress * progress;
    } else if (progress < 2.0f / d1) {
        progress -= 1.5f / d1;
        return n1 * progress * progress + 0.75f;
    } else if (progress < 2.5f / d1) {
        progress -= 2.25f / d1;
        return n1 * progress * progress + 0.9375f;
    } else {
        progress -= 2.625f / d1;
        return n1 * progress * progress + 0.984375f;
    }
}

/**
 * @brief  应用缓动函数
 * @param  progress: 原始进度 (0.0-1.0)
 * @param  easing: 缓动类型
 * @retval 处理后的进度
 */
static float apply_easing(float progress, menu_easing_type_t easing)
{
    switch (easing) {
        case MENU_EASING_LINEAR:
            return easing_linear(progress);

        case MENU_EASING_EASE_IN:
            return easing_ease_in(progress);

        case MENU_EASING_EASE_OUT:
            return easing_ease_out(progress);

        case MENU_EASING_EASE_IN_OUT:
            return easing_ease_in_out(progress);

        case MENU_EASING_BOUNCE:
            return easing_bounce(progress);

        default:
            return progress;
    }
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  初始化动画系统
 */
void menu_anim_init(void)
{
    current_anim.type = MENU_ANIM_NONE;
    current_anim.easing = MENU_EASING_LINEAR;
    current_anim.start_time = 0;
    current_anim.duration = 0;
    current_anim.start_value = 0;
    current_anim.end_value = 0;
    current_anim.is_playing = 0;
}

/**
 * @brief  启动动画
 */
void menu_anim_start(menu_anim_type_t type, menu_easing_type_t easing,
                     uint32_t duration, int16_t start_val, int16_t end_val)
{
    current_anim.type = type;
    current_anim.easing = easing;
    current_anim.start_time = bsp_ec11_get_tick();
    current_anim.duration = duration;
    current_anim.start_value = start_val;
    current_anim.end_value = end_val;
    current_anim.is_playing = 1;
}

/**
 * @brief  更新动画
 */
void menu_anim_update(void)
{
    uint32_t current_time;
    uint32_t elapsed;
    float progress;
    float eased_progress;

    if (!current_anim.is_playing) {
        return;
    }

    current_time = bsp_ec11_get_tick();
    elapsed = current_time - current_anim.start_time;

    /* 检查动画是否结束 */
    if (elapsed >= current_anim.duration) {
        current_anim.is_playing = 0;
        return;
    }

    /* 计算进度 (0.0-1.0) */
    progress = (float)elapsed / (float)current_anim.duration;

    /* 应用缓动函数 */
    eased_progress = apply_easing(progress, current_anim.easing);

    /* 进度被用于menu_anim_get_value()计算当前值 */
    (void)eased_progress;  /* 避免未使用警告 */
}

/**
 * @brief  停止动画
 */
void menu_anim_stop(void)
{
    current_anim.is_playing = 0;
}

/**
 * @brief  获取当前动画值
 */
int16_t menu_anim_get_value(void)
{
    uint32_t current_time;
    uint32_t elapsed;
    float progress;
    float eased_progress;
    int16_t value;

    if (!current_anim.is_playing) {
        return current_anim.end_value;
    }

    current_time = bsp_ec11_get_tick();
    elapsed = current_time - current_anim.start_time;

    /* 检查动画是否结束 */
    if (elapsed >= current_anim.duration) {
        return current_anim.end_value;
    }

    /* 计算进度 */
    progress = (float)elapsed / (float)current_anim.duration;

    /* 应用缓动 */
    eased_progress = apply_easing(progress, current_anim.easing);

    /* 计算当前值 */
    value = current_anim.start_value +
            (int16_t)((current_anim.end_value - current_anim.start_value) * eased_progress);

    return value;
}

/**
 * @brief  获取当前动画进度
 */
uint8_t menu_anim_get_progress(void)
{
    uint32_t current_time;
    uint32_t elapsed;
    uint8_t progress;

    if (!current_anim.is_playing) {
        return 100;
    }

    current_time = bsp_ec11_get_tick();
    elapsed = current_time - current_anim.start_time;

    if (elapsed >= current_anim.duration) {
        return 100;
    }

    progress = (uint8_t)((elapsed * 100) / current_anim.duration);

    return progress;
}

/**
 * @brief  检查动画是否在播放
 */
uint8_t menu_anim_is_playing(void)
{
    return current_anim.is_playing;
}

/**
 * @brief  获取Alpha值
 */
uint8_t menu_anim_get_alpha(void)
{
    int16_t value = menu_anim_get_value();

    /* 限制范围 0-255 */
    if (value < 0) return 0;
    if (value > 255) return 255;

    return (uint8_t)value;
}

/**
 * @brief  获取缩放因子
 */
uint8_t menu_anim_get_scale(void)
{
    int16_t value = menu_anim_get_value();

    /* 限制范围 0-200 */
    if (value < 0) return 0;
    if (value > 200) return 200;

    return (uint8_t)value;
}
