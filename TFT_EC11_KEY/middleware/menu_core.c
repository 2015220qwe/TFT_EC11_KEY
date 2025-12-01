/**
  ******************************************************************************
  * @file    menu_core.c
  * @author  TFT_EC11_KEY Project
  * @brief   通用菜单系统核心实现 - 硬件无关层
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "menu_core.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static menu_state_t menu_state = {0};
static menu_display_callback_t display_callback = NULL;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  获取当前菜单项数组
 * @param  None
 * @retvalue 当前菜单项数组指针
 */
static menu_item_t** menu_get_current_items(void)
{
    if (menu_state.depth >= MENU_MAX_DEPTH) {
        return NULL;
    }

    return menu_state.stack[menu_state.depth];
}

/**
 * @brief  获取当前菜单项数量
 * @param  None
 * @retval 当前菜单项数量
 */
static uint8_t menu_get_current_count(void)
{
    if (menu_state.depth >= MENU_MAX_DEPTH) {
        return 0;
    }

    return menu_state.count_stack[menu_state.depth];
}

/**
 * @brief  获取当前选中索引
 * @param  None
 * @retval 当前选中索引
 */
static uint8_t menu_get_current_index(void)
{
    if (menu_state.depth >= MENU_MAX_DEPTH) {
        return 0;
    }

    return menu_state.index_stack[menu_state.depth];
}

/**
 * @brief  设置当前选中索引
 * @param  index: 索引值
 * @retval None
 */
static void menu_set_current_index(uint8_t index)
{
    if (menu_state.depth < MENU_MAX_DEPTH) {
        menu_state.index_stack[menu_state.depth] = index;
    }
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  菜单系统初始化
 * @param  root_items: 根菜单项数组
 * @param  root_count: 根菜单项数量
 * @param  disp_callback: 显示回调函数
 * @retval None
 */
void menu_init(menu_item_t **root_items, uint8_t root_count,
               menu_display_callback_t disp_callback)
{
    memset(&menu_state, 0, sizeof(menu_state_t));

    menu_state.stack[0] = root_items;
    menu_state.count_stack[0] = root_count;
    menu_state.index_stack[0] = 0;
    menu_state.depth = 0;
    menu_state.scroll_offset = 0;
    menu_state.edit_mode = 0;

    display_callback = disp_callback;

    /* 初始刷新显示 */
    if (display_callback != NULL) {
        display_callback(&menu_state);
    }
}

/**
 * @brief  菜单向上移动
 * @param  None
 * @retval None
 */
void menu_move_up(void)
{
    uint8_t current_index = menu_get_current_index();

    if (current_index > 0) {
        menu_set_current_index(current_index - 1);

        /* 更新滚动偏移 */
        if (current_index - 1 < menu_state.scroll_offset) {
            menu_state.scroll_offset = current_index - 1;
        }

        menu_refresh();
    }
}

/**
 * @brief  菜单向下移动
 * @param  None
 * @retval None
 */
void menu_move_down(void)
{
    uint8_t current_index = menu_get_current_index();
    uint8_t current_count = menu_get_current_count();

    if (current_index < current_count - 1) {
        menu_set_current_index(current_index + 1);

        /* 更新滚动偏移 */
        if (current_index + 1 >= menu_state.scroll_offset + MENU_MAX_ITEMS_PER_PAGE) {
            menu_state.scroll_offset = current_index + 2 - MENU_MAX_ITEMS_PER_PAGE;
        }

        menu_refresh();
    }
}

/**
 * @brief  菜单确认/进入
 * @param  None
 * @retval None
 */
void menu_enter(void)
{
    menu_item_t **current_items = menu_get_current_items();
    uint8_t current_index = menu_get_current_index();

    if (current_items == NULL) {
        return;
    }

    menu_item_t *item = current_items[current_index];

    /* 根据菜单项类型执行操作 */
    switch (item->type) {
        case MENU_ITEM_TYPE_ACTION:
            /* 执行回调函数 */
            if (item->data.action.callback != NULL) {
                item->data.action.callback(item);
            }
            break;

        case MENU_ITEM_TYPE_SUBMENU:
            /* 进入子菜单 */
            if (menu_state.depth < MENU_MAX_DEPTH - 1) {
                menu_state.depth++;
                menu_state.stack[menu_state.depth] = item->data.submenu.items;
                menu_state.count_stack[menu_state.depth] = item->data.submenu.count;
                menu_state.index_stack[menu_state.depth] = 0;
                menu_state.scroll_offset = 0;
                menu_refresh();
            }
            break;

        case MENU_ITEM_TYPE_VALUE:
            /* 进入编辑模式 */
            menu_state.edit_mode = 1;
            menu_refresh();
            break;

        case MENU_ITEM_TYPE_SWITCH:
            /* 切换开关状态 */
            if (item->data.switch_item.state != NULL) {
                *item->data.switch_item.state = !(*item->data.switch_item.state);

                /* 触发回调 */
                if (item->data.switch_item.callback != NULL) {
                    item->data.switch_item.callback(item, *item->data.switch_item.state);
                }

                menu_refresh();
            }
            break;

        default:
            break;
    }
}

/**
 * @brief  菜单返回/退出
 * @param  None
 * @retval None
 */
void menu_back(void)
{
    if (menu_state.edit_mode) {
        /* 退出编辑模式 */
        menu_state.edit_mode = 0;
        menu_refresh();
    } else if (menu_state.depth > 0) {
        /* 返回上一级菜单 */
        menu_state.depth--;
        menu_state.scroll_offset = 0;
        menu_refresh();
    }
}

/**
 * @brief  数值递增
 * @param  None
 * @retval None
 */
void menu_value_increase(void)
{
    if (!menu_state.edit_mode) {
        return;
    }

    menu_item_t **current_items = menu_get_current_items();
    uint8_t current_index = menu_get_current_index();

    if (current_items == NULL) {
        return;
    }

    menu_item_t *item = current_items[current_index];

    if (item->type == MENU_ITEM_TYPE_VALUE && item->data.value.value != NULL) {
        int32_t *val = item->data.value.value;
        int32_t new_val = *val + item->data.value.step;

        if (new_val <= item->data.value.max) {
            *val = new_val;

            /* 触发回调 */
            if (item->data.value.callback != NULL) {
                item->data.value.callback(item, *val);
            }

            menu_refresh();
        }
    }
}

/**
 * @brief  数值递减
 * @param  None
 * @retval None
 */
void menu_value_decrease(void)
{
    if (!menu_state.edit_mode) {
        return;
    }

    menu_item_t **current_items = menu_get_current_items();
    uint8_t current_index = menu_get_current_index();

    if (current_items == NULL) {
        return;
    }

    menu_item_t *item = current_items[current_index];

    if (item->type == MENU_ITEM_TYPE_VALUE && item->data.value.value != NULL) {
        int32_t *val = item->data.value.value;
        int32_t new_val = *val - item->data.value.step;

        if (new_val >= item->data.value.min) {
            *val = new_val;

            /* 触发回调 */
            if (item->data.value.callback != NULL) {
                item->data.value.callback(item, *val);
            }

            menu_refresh();
        }
    }
}

/**
 * @brief  获取当前菜单状态
 * @param  None
 * @retval 菜单状态指针
 */
menu_state_t* menu_get_state(void)
{
    return &menu_state;
}

/**
 * @brief  获取当前选中的菜单项
 * @param  None
 * @retval 菜单项指针
 */
menu_item_t* menu_get_current_item(void)
{
    menu_item_t **current_items = menu_get_current_items();
    uint8_t current_index = menu_get_current_index();

    if (current_items == NULL) {
        return NULL;
    }

    return current_items[current_index];
}

/**
 * @brief  刷新菜单显示
 * @param  None
 * @retval None
 */
void menu_refresh(void)
{
    if (display_callback != NULL) {
        display_callback(&menu_state);
    }
}

/**
 * @brief  获取当前菜单层级
 * @param  None
 * @retval 层级深度
 */
uint8_t menu_get_depth(void)
{
    return menu_state.depth;
}
