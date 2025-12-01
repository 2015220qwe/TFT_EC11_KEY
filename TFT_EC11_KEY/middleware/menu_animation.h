/**
  ******************************************************************************
  * @file    menu_animation.h
  * @author  TFT_EC11_KEY Project
  * @brief   菜单动画效果模块 - 提升用户体验
  ******************************************************************************
  * @attention
  *
  * 支持的动画效果:
  * - 滑动动画 (菜单切换时平滑滑动)
  * - 淡入淡出 (菜单项渐显/渐隐)
  * - 缩放动画 (选中项放大效果)
  * - 弹跳动画 (确认操作时的反馈)
  *
  ******************************************************************************
  */

#ifndef __MENU_ANIMATION_H
#define __MENU_ANIMATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* 动画配置 ------------------------------------------------------------------*/
#define MENU_ANIM_FPS               30     /* 动画帧率 */
#define MENU_ANIM_SLIDE_DURATION    300    /* 滑动动画时长(ms) */
#define MENU_ANIM_FADE_DURATION     200    /* 淡入淡出时长(ms) */
#define MENU_ANIM_BOUNCE_DURATION   150    /* 弹跳动画时长(ms) */

/* 动画类型 ------------------------------------------------------------------*/
typedef enum {
    MENU_ANIM_NONE = 0,        /* 无动画 */
    MENU_ANIM_SLIDE_UP,        /* 向上滑动 */
    MENU_ANIM_SLIDE_DOWN,      /* 向下滑动 */
    MENU_ANIM_SLIDE_LEFT,      /* 向左滑动 */
    MENU_ANIM_SLIDE_RIGHT,     /* 向右滑动 */
    MENU_ANIM_FADE_IN,         /* 淡入 */
    MENU_ANIM_FADE_OUT,        /* 淡出 */
    MENU_ANIM_SCALE_UP,        /* 放大 */
    MENU_ANIM_SCALE_DOWN,      /* 缩小 */
    MENU_ANIM_BOUNCE           /* 弹跳 */
} menu_anim_type_t;

/* 缓动函数类型 --------------------------------------------------------------*/
typedef enum {
    MENU_EASING_LINEAR = 0,    /* 线性 */
    MENU_EASING_EASE_IN,       /* 加速 */
    MENU_EASING_EASE_OUT,      /* 减速 */
    MENU_EASING_EASE_IN_OUT,   /* 先加速后减速 */
    MENU_EASING_BOUNCE         /* 弹跳效果 */
} menu_easing_type_t;

/* 动画状态结构体 ------------------------------------------------------------*/
typedef struct {
    menu_anim_type_t   type;           /* 动画类型 */
    menu_easing_type_t easing;         /* 缓动函数 */
    uint32_t           start_time;     /* 开始时间(ms) */
    uint32_t           duration;       /* 持续时间(ms) */
    int16_t            start_value;    /* 起始值 */
    int16_t            end_value;      /* 结束值 */
    uint8_t            is_playing;     /* 是否正在播放 */
} menu_animation_t;

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  初始化动画系统
 * @param  None
 * @retval None
 */
void menu_anim_init(void);

/**
 * @brief  启动动画
 * @param  type: 动画类型
 * @param  easing: 缓动函数
 * @param  duration: 持续时间(ms)
 * @param  start_val: 起始值
 * @param  end_val: 结束值
 * @retval None
 */
void menu_anim_start(menu_anim_type_t type, menu_easing_type_t easing,
                     uint32_t duration, int16_t start_val, int16_t end_val);

/**
 * @brief  更新动画(需要周期调用)
 * @param  None
 * @retval None
 */
void menu_anim_update(void);

/**
 * @brief  停止动画
 * @param  None
 * @retval None
 */
void menu_anim_stop(void);

/**
 * @brief  获取当前动画值
 * @param  None
 * @retval 当前插值
 */
int16_t menu_anim_get_value(void);

/**
 * @brief  获取当前动画进度 (0-100)
 * @param  None
 * @retval 进度百分比
 */
uint8_t menu_anim_get_progress(void);

/**
 * @brief  检查动画是否在播放
 * @param  None
 * @retval 1-播放中, 0-已停止
 */
uint8_t menu_anim_is_playing(void);

/**
 * @brief  获取Alpha值 (用于淡入淡出)
 * @param  None
 * @retval Alpha值 (0-255)
 */
uint8_t menu_anim_get_alpha(void);

/**
 * @brief  获取缩放因子 (用于缩放动画)
 * @param  None
 * @retval 缩放因子 (0-100, 100表示原始大小)
 */
uint8_t menu_anim_get_scale(void);

/* 辅助宏定义 ----------------------------------------------------------------*/

/* 启动菜单切换滑动动画 */
#define MENU_ANIM_START_SLIDE_UP() \
    menu_anim_start(MENU_ANIM_SLIDE_UP, MENU_EASING_EASE_OUT, \
                    MENU_ANIM_SLIDE_DURATION, 0, -100)

#define MENU_ANIM_START_SLIDE_DOWN() \
    menu_anim_start(MENU_ANIM_SLIDE_DOWN, MENU_EASING_EASE_OUT, \
                    MENU_ANIM_SLIDE_DURATION, 0, 100)

/* 启动淡入淡出动画 */
#define MENU_ANIM_START_FADE_IN() \
    menu_anim_start(MENU_ANIM_FADE_IN, MENU_EASING_EASE_IN, \
                    MENU_ANIM_FADE_DURATION, 0, 255)

#define MENU_ANIM_START_FADE_OUT() \
    menu_anim_start(MENU_ANIM_FADE_OUT, MENU_EASING_EASE_OUT, \
                    MENU_ANIM_FADE_DURATION, 255, 0)

/* 启动弹跳动画 */
#define MENU_ANIM_START_BOUNCE() \
    menu_anim_start(MENU_ANIM_BOUNCE, MENU_EASING_BOUNCE, \
                    MENU_ANIM_BOUNCE_DURATION, 0, 10)

#ifdef __cplusplus
}
#endif

#endif /* __MENU_ANIMATION_H */
