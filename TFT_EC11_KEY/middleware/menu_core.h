/**
  ******************************************************************************
  * @file    menu_core.h
  * @author  TFT_EC11_KEY Project
  * @brief   通用菜单系统核心 - 硬件无关层
  *          支持多级菜单、回调函数、可配置菜单项
  ******************************************************************************
  */

#ifndef __MENU_CORE_H
#define __MENU_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/* 菜单配置 ------------------------------------------------------------------*/
#define MENU_MAX_ITEMS_PER_PAGE   6    /* 每页最多显示菜单项数 */
#define MENU_MAX_DEPTH            4    /* 最大菜单层级深度 */

/* 菜单项类型 ----------------------------------------------------------------*/
typedef enum {
    MENU_ITEM_TYPE_ACTION = 0,  /* 执行动作(调用回调函数) */
    MENU_ITEM_TYPE_SUBMENU,     /* 进入子菜单 */
    MENU_ITEM_TYPE_VALUE,       /* 数值调节项 */
    MENU_ITEM_TYPE_SWITCH       /* 开关项 */
} menu_item_type_t;

/* 前向声明 ------------------------------------------------------------------*/
typedef struct menu_item_s menu_item_t;

/* 菜单项回调函数类型 --------------------------------------------------------*/
typedef void (*menu_action_callback_t)(menu_item_t *item);
typedef void (*menu_value_changed_callback_t)(menu_item_t *item, int32_t new_value);

/* 菜单项结构体 --------------------------------------------------------------*/
struct menu_item_s {
    char                *name;           /* 菜单项名称 */
    menu_item_type_t     type;           /* 菜单项类型 */

    union {
        /* ACTION类型 */
        struct {
            menu_action_callback_t callback;
        } action;

        /* SUBMENU类型 */
        struct {
            menu_item_t **items;   /* 子菜单项数组 */
            uint8_t       count;   /* 子菜单项数量 */
        } submenu;

        /* VALUE类型 */
        struct {
            int32_t  *value;       /* 数值指针 */
            int32_t   min;         /* 最小值 */
            int32_t   max;         /* 最大值 */
            int32_t   step;        /* 步进值 */
            menu_value_changed_callback_t callback;
        } value;

        /* SWITCH类型 */
        struct {
            uint8_t *state;        /* 开关状态指针 (0/1) */
            menu_value_changed_callback_t callback;
        } switch_item;
    } data;

    void *user_data;  /* 用户自定义数据 */
};

/* 菜单状态结构体 ------------------------------------------------------------*/
typedef struct {
    menu_item_t **stack[MENU_MAX_DEPTH];  /* 菜单栈(保存各层菜单项数组) */
    uint8_t       count_stack[MENU_MAX_DEPTH]; /* 各层菜单项数量 */
    uint8_t       index_stack[MENU_MAX_DEPTH]; /* 各层当前选中索引 */
    uint8_t       depth;                  /* 当前菜单深度 */
    uint8_t       scroll_offset;          /* 滚动偏移 */
    uint8_t       edit_mode;              /* 编辑模式标志 (用于数值调节) */
} menu_state_t;

/* 菜单显示回调函数类型 ------------------------------------------------------*/
typedef void (*menu_display_callback_t)(menu_state_t *state);

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  菜单系统初始化
 * @param  root_items: 根菜单项数组
 * @param  root_count: 根菜单项数量
 * @param  display_callback: 显示回调函数
 * @retval None
 */
void menu_init(menu_item_t **root_items, uint8_t root_count,
               menu_display_callback_t display_callback);

/**
 * @brief  菜单向上移动
 * @param  None
 * @retval None
 */
void menu_move_up(void);

/**
 * @brief  菜单向下移动
 * @param  None
 * @retval None
 */
void menu_move_down(void);

/**
 * @brief  菜单确认/进入
 * @param  None
 * @retval None
 */
void menu_enter(void);

/**
 * @brief  菜单返回/退出
 * @param  None
 * @retval None
 */
void menu_back(void);

/**
 * @brief  数值递增 (仅在编辑模式有效)
 * @param  None
 * @retval None
 */
void menu_value_increase(void);

/**
 * @brief  数值递减 (仅在编辑模式有效)
 * @param  None
 * @retval None
 */
void menu_value_decrease(void);

/**
 * @brief  获取当前菜单状态
 * @param  None
 * @retval 菜单状态指针
 */
menu_state_t* menu_get_state(void);

/**
 * @brief  获取当前选中的菜单项
 * @param  None
 * @retval 菜单项指针
 */
menu_item_t* menu_get_current_item(void);

/**
 * @brief  刷新菜单显示
 * @param  None
 * @retval None
 */
void menu_refresh(void);

/**
 * @brief  获取当前菜单层级
 * @param  None
 * @retval 层级深度 (0-根菜单)
 */
uint8_t menu_get_depth(void);

#ifdef __cplusplus
}
#endif

#endif /* __MENU_CORE_H */
