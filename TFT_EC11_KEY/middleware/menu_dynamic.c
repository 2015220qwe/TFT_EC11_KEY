/**
  ******************************************************************************
  * @file    menu_dynamic.c
  * @author  TFT_EC11_KEY Project
  * @brief   菜单项动态管理实现
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "menu_dynamic.h"
#include <string.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
static menu_item_ex_t item_pool[MENU_DYNAMIC_POOL_SIZE];
static uint8_t pool_used[MENU_DYNAMIC_POOL_SIZE] = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  从池中分配菜单项
 * @param  None
 * @retval 菜单项指针, NULL表示池已满
 */
static menu_item_ex_t* alloc_item_from_pool(void)
{
    uint8_t i;

    for (i = 0; i < MENU_DYNAMIC_POOL_SIZE; i++) {
        if (!pool_used[i]) {
            pool_used[i] = 1;
            memset(&item_pool[i], 0, sizeof(menu_item_ex_t));
            item_pool[i].flags = MENU_ITEM_FLAG_VISIBLE | MENU_ITEM_FLAG_ENABLED | MENU_ITEM_FLAG_DYNAMIC;
            return &item_pool[i];
        }
    }

    return NULL;  /* 池已满 */
}

/**
 * @brief  释放菜单项到池
 * @param  item: 菜单项指针
 * @retval None
 */
static void free_item_to_pool(menu_item_ex_t *item)
{
    uint32_t index = item - item_pool;

    if (index < MENU_DYNAMIC_POOL_SIZE) {
        /* 释放动态分配的名称 */
        if (item->base.name != NULL) {
            free(item->base.name);
        }

        pool_used[index] = 0;
    }
}

/**
 * @brief  获取扩展菜单项
 * @param  item: 基础菜单项指针
 * @retval 扩展菜单项指针, NULL表示不是动态菜单项
 */
static menu_item_ex_t* get_ex_item(menu_item_t *item)
{
    uint32_t index;
    menu_item_ex_t *ex_item;

    if (item == NULL) {
        return NULL;
    }

    /* 检查是否在池中 */
    ex_item = (menu_item_ex_t*)((char*)item - offsetof(menu_item_ex_t, base));
    index = ex_item - item_pool;

    if (index < MENU_DYNAMIC_POOL_SIZE && pool_used[index]) {
        return ex_item;
    }

    return NULL;
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  初始化动态菜单系统
 */
void menu_dynamic_init(void)
{
    memset(item_pool, 0, sizeof(item_pool));
    memset(pool_used, 0, sizeof(pool_used));
}

/**
 * @brief  创建新的菜单项
 */
menu_item_t* menu_dynamic_create_item(const char *name, menu_item_type_t type)
{
    menu_item_ex_t *ex_item;
    char *name_copy;

    /* 从池中分配 */
    ex_item = alloc_item_from_pool();
    if (ex_item == NULL) {
        return NULL;
    }

    /* 复制名称 */
    name_copy = (char*)malloc(strlen(name) + 1);
    if (name_copy == NULL) {
        free_item_to_pool(ex_item);
        return NULL;
    }
    strcpy(name_copy, name);

    /* 初始化基础菜单项 */
    ex_item->base.name = name_copy;
    ex_item->base.type = type;

    return &ex_item->base;
}

/**
 * @brief  删除菜单项
 */
int menu_dynamic_delete_item(menu_item_t *item)
{
    menu_item_ex_t *ex_item;

    ex_item = get_ex_item(item);
    if (ex_item == NULL) {
        return -1;  /* 不是动态菜单项 */
    }

    if (!(ex_item->flags & MENU_ITEM_FLAG_DYNAMIC)) {
        return -1;  /* 不可删除 */
    }

    /* 释放到池 */
    free_item_to_pool(ex_item);

    return 0;
}

/**
 * @brief  添加菜单项到父菜单
 */
int menu_dynamic_add_item(menu_item_t *parent, menu_item_t *item, int index)
{
    menu_item_t **new_items;
    uint8_t old_count, new_count;
    uint8_t i;

    if (parent == NULL || item == NULL) {
        return -1;
    }

    if (parent->type != MENU_ITEM_TYPE_SUBMENU) {
        return -1;  /* 父菜单必须是SUBMENU类型 */
    }

    old_count = parent->data.submenu.count;
    new_count = old_count + 1;

    /* 重新分配菜单项数组 */
    new_items = (menu_item_t**)malloc(new_count * sizeof(menu_item_t*));
    if (new_items == NULL) {
        return -1;
    }

    /* 复制现有项 */
    if (index < 0 || index >= old_count) {
        /* 添加到末尾 */
        if (old_count > 0) {
            memcpy(new_items, parent->data.submenu.items, old_count * sizeof(menu_item_t*));
        }
        new_items[old_count] = item;
    } else {
        /* 插入到指定位置 */
        for (i = 0; i < index; i++) {
            new_items[i] = parent->data.submenu.items[i];
        }
        new_items[index] = item;
        for (i = index; i < old_count; i++) {
            new_items[i + 1] = parent->data.submenu.items[i];
        }
    }

    /* 释放旧数组 */
    if (parent->data.submenu.items != NULL) {
        free(parent->data.submenu.items);
    }

    /* 更新父菜单 */
    parent->data.submenu.items = new_items;
    parent->data.submenu.count = new_count;

    return 0;
}

/**
 * @brief  从父菜单移除菜单项
 */
int menu_dynamic_remove_item(menu_item_t *parent, menu_item_t *item)
{
    menu_item_t **new_items;
    uint8_t old_count, new_count;
    uint8_t i, j;
    int found_index = -1;

    if (parent == NULL || item == NULL) {
        return -1;
    }

    if (parent->type != MENU_ITEM_TYPE_SUBMENU) {
        return -1;
    }

    old_count = parent->data.submenu.count;

    /* 查找项 */
    for (i = 0; i < old_count; i++) {
        if (parent->data.submenu.items[i] == item) {
            found_index = i;
            break;
        }
    }

    if (found_index < 0) {
        return -1;  /* 未找到 */
    }

    new_count = old_count - 1;

    if (new_count == 0) {
        /* 移除所有项 */
        free(parent->data.submenu.items);
        parent->data.submenu.items = NULL;
        parent->data.submenu.count = 0;
        return 0;
    }

    /* 分配新数组 */
    new_items = (menu_item_t**)malloc(new_count * sizeof(menu_item_t*));
    if (new_items == NULL) {
        return -1;
    }

    /* 复制项(跳过要删除的) */
    for (i = 0, j = 0; i < old_count; i++) {
        if (i != found_index) {
            new_items[j++] = parent->data.submenu.items[i];
        }
    }

    /* 释放旧数组 */
    free(parent->data.submenu.items);

    /* 更新父菜单 */
    parent->data.submenu.items = new_items;
    parent->data.submenu.count = new_count;

    return 0;
}

/**
 * @brief  设置菜单项可见性
 */
void menu_dynamic_set_visible(menu_item_t *item, uint8_t visible)
{
    menu_item_ex_t *ex_item = get_ex_item(item);

    if (ex_item != NULL) {
        if (visible) {
            ex_item->flags |= MENU_ITEM_FLAG_VISIBLE;
        } else {
            ex_item->flags &= ~MENU_ITEM_FLAG_VISIBLE;
        }
    }
}

/**
 * @brief  获取菜单项可见性
 */
uint8_t menu_dynamic_is_visible(menu_item_t *item)
{
    menu_item_ex_t *ex_item = get_ex_item(item);

    if (ex_item != NULL) {
        return (ex_item->flags & MENU_ITEM_FLAG_VISIBLE) ? 1 : 0;
    }

    return 1;  /* 默认可见 */
}

/**
 * @brief  设置菜单项启用状态
 */
void menu_dynamic_set_enabled(menu_item_t *item, uint8_t enabled)
{
    menu_item_ex_t *ex_item = get_ex_item(item);

    if (ex_item != NULL) {
        if (enabled) {
            ex_item->flags |= MENU_ITEM_FLAG_ENABLED;
        } else {
            ex_item->flags &= ~MENU_ITEM_FLAG_ENABLED;
        }
    }
}

/**
 * @brief  获取菜单项启用状态
 */
uint8_t menu_dynamic_is_enabled(menu_item_t *item)
{
    menu_item_ex_t *ex_item = get_ex_item(item);

    if (ex_item != NULL) {
        return (ex_item->flags & MENU_ITEM_FLAG_ENABLED) ? 1 : 0;
    }

    return 1;  /* 默认启用 */
}

/**
 * @brief  查找菜单项
 */
menu_item_t* menu_dynamic_find_item(menu_item_t *parent, const char *name)
{
    uint8_t i;

    if (parent == NULL || name == NULL) {
        return NULL;
    }

    if (parent->type != MENU_ITEM_TYPE_SUBMENU) {
        return NULL;
    }

    for (i = 0; i < parent->data.submenu.count; i++) {
        if (strcmp(parent->data.submenu.items[i]->name, name) == 0) {
            return parent->data.submenu.items[i];
        }
    }

    return NULL;
}

/**
 * @brief  获取菜单项数量
 */
uint8_t menu_dynamic_get_item_count(menu_item_t *parent)
{
    uint8_t i, count = 0;

    if (parent == NULL) {
        return 0;
    }

    if (parent->type != MENU_ITEM_TYPE_SUBMENU) {
        return 0;
    }

    /* 统计可见项 */
    for (i = 0; i < parent->data.submenu.count; i++) {
        if (menu_dynamic_is_visible(parent->data.submenu.items[i])) {
            count++;
        }
    }

    return count;
}

/**
 * @brief  清空动态菜单池
 */
void menu_dynamic_clear_pool(void)
{
    uint8_t i;

    for (i = 0; i < MENU_DYNAMIC_POOL_SIZE; i++) {
        if (pool_used[i]) {
            free_item_to_pool(&item_pool[i]);
        }
    }
}
