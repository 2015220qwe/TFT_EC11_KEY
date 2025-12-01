/**
  ******************************************************************************
  * @file    main_example.c
  * @brief   TFT_EC11_KEY菜单系统完整示例
  * @note    演示如何使用EC11编码器、按键和菜单系统
  ******************************************************************************
  */

#include "stm32f4xx.h"
#include "board.h"
#include "bsp_ec11.h"
#include "bsp_key.h"
#include "menu_core.h"
#include <stdio.h>

/* 全局变量 ----------------------------------------------------------------*/
static volatile uint32_t system_tick = 0;  /* 系统滴答计数器 */

/* 菜单相关变量 */
static int32_t brightness = 50;    /* 亮度值 (0-100) */
static int32_t volume = 30;        /* 音量值 (0-100) */
static uint8_t wifi_enable = 1;    /* WiFi开关 */
static uint8_t bluetooth_enable = 0; /* 蓝牙开关 */

/* 函数声明 ----------------------------------------------------------------*/
void systick_init(void);
void led_toggle(void);
void menu_display_callback(menu_state_t *state);

/* 系统时间戳函数实现 ------------------------------------------------------*/
uint32_t bsp_ec11_get_tick(void)
{
    return system_tick;
}

/* SysTick中断服务函数 ----------------------------------------------------*/
void SysTick_Handler(void)
{
    system_tick++;
}

/* SysTick初始化 (1ms中断) ------------------------------------------------*/
void systick_init(void)
{
    /* 配置SysTick为1ms中断 (假设系统时钟168MHz) */
    SysTick_Config(SystemCoreClock / 1000);
}

/* LED测试函数 -------------------------------------------------------------*/
void led_toggle(void)
{
    static uint8_t led_state = 0;
    led_state = !led_state;

    /* TODO: 根据实际硬件实现LED切换 */
    printf("LED Toggle: %d\n", led_state);
}

/* 菜单回调函数 ------------------------------------------------------------*/
void menu_action_led_test(menu_item_t *item)
{
    printf("执行: LED测试\n");
    led_toggle();
}

void menu_action_system_info(menu_item_t *item)
{
    printf("执行: 显示系统信息\n");
    printf("固件版本: V1.0.0\n");
    printf("编译日期: %s %s\n", __DATE__, __TIME__);
}

void menu_brightness_changed(menu_item_t *item, int32_t new_value)
{
    printf("亮度调节: %ld%%\n", new_value);
    /* TODO: 实际的亮度调节代码 */
}

void menu_volume_changed(menu_item_t *item, int32_t new_value)
{
    printf("音量调节: %ld%%\n", new_value);
    /* TODO: 实际的音量调节代码 */
}

void menu_wifi_changed(menu_item_t *item, int32_t new_state)
{
    printf("WiFi: %s\n", new_state ? "开启" : "关闭");
    /* TODO: 实际的WiFi控制代码 */
}

void menu_bluetooth_changed(menu_item_t *item, int32_t new_state)
{
    printf("蓝牙: %s\n", new_state ? "开启" : "关闭");
    /* TODO: 实际的蓝牙控制代码 */
}

/* 菜单结构定义 ------------------------------------------------------------*/

/* 设置子菜单项 */
menu_item_t menu_brightness = {
    .name = "亮度",
    .type = MENU_ITEM_TYPE_VALUE,
    .data.value = {
        .value = &brightness,
        .min = 0,
        .max = 100,
        .step = 5,
        .callback = menu_brightness_changed
    }
};

menu_item_t menu_volume = {
    .name = "音量",
    .type = MENU_ITEM_TYPE_VALUE,
    .data.value = {
        .value = &volume,
        .min = 0,
        .max = 100,
        .step = 10,
        .callback = menu_volume_changed
    }
};

menu_item_t menu_wifi = {
    .name = "WiFi",
    .type = MENU_ITEM_TYPE_SWITCH,
    .data.switch_item = {
        .state = &wifi_enable,
        .callback = menu_wifi_changed
    }
};

menu_item_t menu_bluetooth = {
    .name = "蓝牙",
    .type = MENU_ITEM_TYPE_SWITCH,
    .data.switch_item = {
        .state = &bluetooth_enable,
        .callback = menu_bluetooth_changed
    }
};

menu_item_t* settings_items[] = {
    &menu_brightness,
    &menu_volume,
    &menu_wifi,
    &menu_bluetooth
};

/* 根菜单项 */
menu_item_t menu_settings = {
    .name = "系统设置",
    .type = MENU_ITEM_TYPE_SUBMENU,
    .data.submenu = {
        .items = settings_items,
        .count = 4
    }
};

menu_item_t menu_led_test = {
    .name = "LED测试",
    .type = MENU_ITEM_TYPE_ACTION,
    .data.action = {
        .callback = menu_action_led_test
    }
};

menu_item_t menu_system_info = {
    .name = "系统信息",
    .type = MENU_ITEM_TYPE_ACTION,
    .data.action = {
        .callback = menu_action_system_info
    }
};

menu_item_t* root_items[] = {
    &menu_settings,
    &menu_led_test,
    &menu_system_info
};

/* EC11事件处理回调 --------------------------------------------------------*/
void ec11_event_handler(ec11_event_t event)
{
    menu_state_t *state = menu_get_state();

    switch (event) {
        case EC11_EVENT_ROTATE_LEFT:
            if (state->edit_mode) {
                menu_value_decrease();  /* 编辑模式: 减小数值 */
            } else {
                menu_move_up();         /* 普通模式: 向上移动 */
            }
            break;

        case EC11_EVENT_ROTATE_RIGHT:
            if (state->edit_mode) {
                menu_value_increase();  /* 编辑模式: 增加数值 */
            } else {
                menu_move_down();       /* 普通模式: 向下移动 */
            }
            break;

        case EC11_EVENT_KEY_SHORT_PRESS:
            menu_enter();  /* 短按: 确认/进入 */
            break;

        case EC11_EVENT_KEY_LONG_PRESS:
            printf("EC11长按检测\n");
            break;

        default:
            break;
    }
}

/* 独立按键事件处理回调 ----------------------------------------------------*/
void key_event_handler(key_id_t key_id, key_event_t event)
{
    if (key_id == KEY_ID_0 && event == KEY_EVENT_SHORT_PRESS) {
        menu_back();  /* KEY0: 返回 */
    }
}

/* 菜单显示回调 (简化版，实际需要配合U8G2实现) ---------------------------*/
void menu_display_callback(menu_state_t *state)
{
    uint8_t i;
    menu_item_t **items = state->stack[state->depth];
    uint8_t count = state->count_stack[state->depth];
    uint8_t current_index = state->index_stack[state->depth];

    printf("\n========== 菜单显示 ==========\n");
    printf("层级: %d\n", state->depth);

    /* 显示可见菜单项 */
    for (i = 0; i < count && i < MENU_MAX_ITEMS_PER_PAGE; i++) {
        uint8_t item_index = state->scroll_offset + i;

        if (item_index >= count) {
            break;
        }

        menu_item_t *item = items[item_index];

        /* 显示选择指示符 */
        if (item_index == current_index) {
            printf("> ");
        } else {
            printf("  ");
        }

        /* 显示菜单项名称 */
        printf("%s", item->name);

        /* 显示菜单项值或状态 */
        switch (item->type) {
            case MENU_ITEM_TYPE_VALUE:
                if (state->edit_mode && item_index == current_index) {
                    printf(": [%ld]", *item->data.value.value);  /* 编辑模式 */
                } else {
                    printf(": %ld", *item->data.value.value);
                }
                break;

            case MENU_ITEM_TYPE_SWITCH:
                printf(": %s", *item->data.switch_item.state ? "开" : "关");
                break;

            case MENU_ITEM_TYPE_SUBMENU:
                printf(" >");
                break;

            default:
                break;
        }

        printf("\n");
    }

    printf("==============================\n\n");
}

/* 主函数 ------------------------------------------------------------------*/
int main(void)
{
    /* 系统初始化 */
    board_init();
    systick_init();

    printf("\n======================================\n");
    printf("  TFT_EC11_KEY 菜单系统示例\n");
    printf("======================================\n\n");

    /* 初始化EC11编码器 */
    bsp_ec11_init();
    bsp_ec11_register_callback(ec11_event_handler);

    /* 初始化独立按键 */
    key_config_t key_configs[] = {
        {GPIOA, GPIO_Pin_3, 0}  /* KEY0: PA3, 低电平有效 */
    };
    bsp_key_init(key_configs, 1);
    bsp_key_register_callback(key_event_handler);

    /* 初始化菜单系统 */
    menu_init(root_items, 3, menu_display_callback);

    printf("初始化完成!\n");
    printf("使用说明:\n");
    printf("- 旋转EC11: 上下移动菜单\n");
    printf("- 短按EC11: 确认/进入\n");
    printf("- 短按KEY0: 返回上一级\n\n");

    /* 主循环 */
    while (1) {
        /* 周期扫描 */
        bsp_ec11_scan();
        bsp_key_scan();

        /* 延时 */
        delay_ms(10);
    }
}
