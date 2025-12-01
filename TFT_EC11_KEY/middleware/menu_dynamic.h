/**
  ******************************************************************************
  * @file    menu_dynamic.h
  * @author  TFT_EC11_KEY Project
  * @brief   菜单项动态管理扩展 - 支持运行时添加/删除菜单项
  ******************************************************************************
  * @attention
  *
  * 功能特性:
  * - 运行时动态添加菜单项
  * - 运行时动态删除菜单项
  * - 菜单项显示/隐藏控制
  * - 菜单项启用/禁用控制
  * - 自动内存管理
  *
  ******************************************************************************
  */

#ifndef __MENU_DYNAMIC_H
#define __MENU_DYNAMIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "menu_core.h"

/* 配置选项 ------------------------------------------------------------------*/
#define MENU_DYNAMIC_POOL_SIZE    32   /* 动态菜单项池大小 */

/* 菜单项属性标志 ------------------------------------------------------------*/
#define MENU_ITEM_FLAG_VISIBLE    (1 << 0)  /* 可见 */
#define MENU_ITEM_FLAG_ENABLED    (1 << 1)  /* 启用 */
#define MENU_ITEM_FLAG_DYNAMIC    (1 << 2)  /* 动态分配(可删除) */

/* 扩展菜单项结构体 ----------------------------------------------------------*/
typedef struct {
    menu_item_t  base;        /* 基础菜单项 */
    uint8_t      flags;       /* 属性标志 */
    void        *user_data;   /* 用户自定义数据 */
} menu_item_ex_t;

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  初始化动态菜单系统
 * @param  None
 * @retval None
 */
void menu_dynamic_init(void);

/**
 * @brief  创建新的菜单项
 * @param  name: 菜单项名称
 * @param  type: 菜单项类型
 * @retval 菜单项指针, NULL表示失败
 */
menu_item_t* menu_dynamic_create_item(const char *name, menu_item_type_t type);

/**
 * @brief  删除菜单项
 * @param  item: 菜单项指针
 * @retval 0-成功, -1-失败
 */
int menu_dynamic_delete_item(menu_item_t *item);

/**
 * @brief  添加菜单项到父菜单
 * @param  parent: 父菜单项(SUBMENU类型)
 * @param  item: 要添加的菜单项
 * @param  index: 插入位置(-1表示末尾)
 * @retval 0-成功, -1-失败
 */
int menu_dynamic_add_item(menu_item_t *parent, menu_item_t *item, int index);

/**
 * @brief  从父菜单移除菜单项
 * @param  parent: 父菜单项
 * @param  item: 要移除的菜单项
 * @retval 0-成功, -1-失败
 */
int menu_dynamic_remove_item(menu_item_t *parent, menu_item_t *item);

/**
 * @brief  设置菜单项可见性
 * @param  item: 菜单项指针
 * @param  visible: 1-可见, 0-隐藏
 * @retval None
 */
void menu_dynamic_set_visible(menu_item_t *item, uint8_t visible);

/**
 * @brief  获取菜单项可见性
 * @param  item: 菜单项指针
 * @retval 1-可见, 0-隐藏
 */
uint8_t menu_dynamic_is_visible(menu_item_t *item);

/**
 * @brief  设置菜单项启用状态
 * @param  item: 菜单项指针
 * @param  enabled: 1-启用, 0-禁用
 * @retval None
 */
void menu_dynamic_set_enabled(menu_item_t *item, uint8_t enabled);

/**
 * @brief  获取菜单项启用状态
 * @param  item: 菜单项指针
 * @retval 1-启用, 0-禁用
 */
uint8_t menu_dynamic_is_enabled(menu_item_t *item);

/**
 * @brief  查找菜单项
 * @param  parent: 父菜单项
 * @param  name: 菜单项名称
 * @retval 菜单项指针, NULL表示未找到
 */
menu_item_t* menu_dynamic_find_item(menu_item_t *parent, const char *name);

/**
 * @brief  获取菜单项数量(仅统计可见项)
 * @param  parent: 父菜单项
 * @retval 菜单项数量
 */
uint8_t menu_dynamic_get_item_count(menu_item_t *parent);

/**
 * @brief  清空动态菜单池
 * @param  None
 * @retval None
 */
void menu_dynamic_clear_pool(void);

/* 便捷宏定义 ----------------------------------------------------------------*/

/* 快速创建ACTION类型菜单项 */
#define MENU_CREATE_ACTION(name, callback) \
    ({ \
        menu_item_t *item = menu_dynamic_create_item(name, MENU_ITEM_TYPE_ACTION); \
        if (item) item->data.action.callback = callback; \
        item; \
    })

/* 快速创建SWITCH类型菜单项 */
#define MENU_CREATE_SWITCH(name, state_ptr, callback) \
    ({ \
        menu_item_t *item = menu_dynamic_create_item(name, MENU_ITEM_TYPE_SWITCH); \
        if (item) { \
            item->data.switch_item.state = state_ptr; \
            item->data.switch_item.callback = callback; \
        } \
        item; \
    })

/* 快速创建VALUE类型菜单项 */
#define MENU_CREATE_VALUE(name, val_ptr, min, max, step, callback) \
    ({ \
        menu_item_t *item = menu_dynamic_create_item(name, MENU_ITEM_TYPE_VALUE); \
        if (item) { \
            item->data.value.value = val_ptr; \
            item->data.value.min = min; \
            item->data.value.max = max; \
            item->data.value.step = step; \
            item->data.value.callback = callback; \
        } \
        item; \
    })

#ifdef __cplusplus
}
#endif

#endif /* __MENU_DYNAMIC_H */
