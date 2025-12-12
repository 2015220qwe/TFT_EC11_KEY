/**
 * @file main_app.c
 * @brief 综合应用示例 - 基于伪调度器的完整演示
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 功能演示:
 *       - EC11旋转编码器控制菜单
 *       - ADC采样并显示波形
 *       - 蓝牙数据透传
 *       - PWM呼吸灯效果
 *       - TFT/OLED显示
 *       - 所有任务由伪调度器管理
 */

#include "stm32f4xx.h"
#include "board.h"

/* BSP驱动 */
#include "bsp/bsp_ec11.h"
#include "bsp/bsp_key.h"
#include "bsp/bsp_adc.h"
#include "bsp/bsp_uart.h"
#include "bsp/bsp_bluetooth.h"
#include "bsp/bsp_pwm.h"
#include "bsp/bsp_timer.h"
#include "bsp/bsp_tft_st7789.h"

/* 中间件 */
#include "middleware/scheduler.h"
#include "middleware/menu_core.h"
#include "middleware/waveform_display.h"

/*=============================================================================
 *                              全局变量
 *============================================================================*/

/* PWM通道 */
static pwm_channel_t led_pwm_channel;

/* ADC波形缓冲区 */
static uint16_t adc_buffer[256];

/* 应用状态 */
typedef enum {
    APP_MODE_MENU,          /* 菜单模式 */
    APP_MODE_OSCILLOSCOPE,  /* 示波器模式 */
    APP_MODE_BLUETOOTH      /* 蓝牙透传模式 */
} app_mode_t;

static app_mode_t current_mode = APP_MODE_MENU;

/* 系统参数 */
static struct {
    uint8_t led_brightness;     /* LED亮度 0-100 */
    uint8_t bt_enable;          /* 蓝牙使能 */
    uint16_t adc_sample_rate;   /* ADC采样率 */
    uint8_t display_brightness; /* 显示亮度 */
} system_params = {
    .led_brightness = 50,
    .bt_enable = 1,
    .adc_sample_rate = 1000,
    .display_brightness = 100
};

/*=============================================================================
 *                              任务函数声明
 *============================================================================*/

static void task_ec11_scan(void *arg);
static void task_key_scan(void *arg);
static void task_adc_sample(void *arg);
static void task_display_update(void *arg);
static void task_bluetooth_process(void *arg);
static void task_led_breath(void *arg);
static void task_system_monitor(void *arg);

/*=============================================================================
 *                              回调函数
 *============================================================================*/

/**
 * @brief EC11事件回调
 */
static void ec11_event_handler(ec11_event_t event)
{
    switch (current_mode) {
    case APP_MODE_MENU:
        switch (event) {
        case EC11_EVENT_ROTATE_LEFT:
            menu_move_up();
            break;
        case EC11_EVENT_ROTATE_RIGHT:
            menu_move_down();
            break;
        case EC11_EVENT_KEY_SHORT_PRESS:
            menu_enter();
            break;
        case EC11_EVENT_KEY_LONG_PRESS:
            menu_back();
            break;
        default:
            break;
        }
        break;

    case APP_MODE_OSCILLOSCOPE:
        switch (event) {
        case EC11_EVENT_ROTATE_LEFT:
            waveform_timebase_decrease();
            break;
        case EC11_EVENT_ROTATE_RIGHT:
            waveform_timebase_increase();
            break;
        case EC11_EVENT_KEY_SHORT_PRESS:
            waveform_toggle_measurement();
            break;
        case EC11_EVENT_KEY_LONG_PRESS:
            current_mode = APP_MODE_MENU;
            waveform_stop();
            break;
        default:
            break;
        }
        break;

    case APP_MODE_BLUETOOTH:
        if (event == EC11_EVENT_KEY_LONG_PRESS) {
            current_mode = APP_MODE_MENU;
        }
        break;
    }
}

/**
 * @brief 蓝牙数据帧回调
 */
static void bluetooth_frame_handler(const bt_frame_t *frame)
{
    switch (frame->cmd) {
    case BT_CMD_SET_PARAM:
        if (frame->len >= 2) {
            uint8_t param_id = frame->data[0];
            uint8_t value = frame->data[1];

            switch (param_id) {
            case 0: /* LED亮度 */
                system_params.led_brightness = value;
                bsp_pwm_set_duty_percent(led_pwm_channel, value);
                break;
            case 1: /* 显示亮度 */
                system_params.display_brightness = value;
                bsp_tft_set_brightness(value);
                break;
            }

            /* 回复ACK */
            bsp_bluetooth_send_frame(BT_CMD_ACK, NULL, 0);
        }
        break;

    case BT_CMD_GET_PARAM:
        {
            uint8_t resp[4];
            resp[0] = system_params.led_brightness;
            resp[1] = system_params.display_brightness;
            resp[2] = (uint8_t)(system_params.adc_sample_rate >> 8);
            resp[3] = (uint8_t)(system_params.adc_sample_rate & 0xFF);
            bsp_bluetooth_send_frame(BT_CMD_GET_PARAM, resp, 4);
        }
        break;

    case BT_CMD_START:
        current_mode = APP_MODE_OSCILLOSCOPE;
        waveform_start();
        break;

    case BT_CMD_STOP:
        waveform_stop();
        break;
    }
}

/**
 * @brief 蓝牙状态变化回调
 */
static void bluetooth_state_handler(bt_state_t state)
{
    if (state == BT_STATE_CONNECTED) {
        DEBUG_PRINT("Bluetooth connected!\r\n");
    } else if (state == BT_STATE_DISCONNECTED) {
        DEBUG_PRINT("Bluetooth disconnected.\r\n");
    }
}

/*=============================================================================
 *                              菜单定义
 *============================================================================*/

/* 菜单动作回调 */
static void menu_action_oscilloscope(void)
{
    current_mode = APP_MODE_OSCILLOSCOPE;
    waveform_start();
}

static void menu_action_bluetooth(void)
{
    current_mode = APP_MODE_BLUETOOTH;
}

static void menu_action_led_test(void)
{
    bsp_pwm_breath_start(led_pwm_channel, NULL);
}

static void menu_value_led_brightness(int32_t value)
{
    system_params.led_brightness = (uint8_t)value;
    bsp_pwm_set_duty_percent(led_pwm_channel, value);
}

static void menu_value_display_brightness(int32_t value)
{
    system_params.display_brightness = (uint8_t)value;
    bsp_tft_set_brightness(value);
}

/* 子菜单项 */
static menu_item_t settings_items[] = {
    {
        .name = "LED Brightness",
        .type = MENU_ITEM_TYPE_VALUE,
        .value.value_ptr = (int32_t*)&system_params.led_brightness,
        .value.min = 0,
        .value.max = 100,
        .value.step = 10,
        .value_changed_callback = menu_value_led_brightness
    },
    {
        .name = "Display Bright",
        .type = MENU_ITEM_TYPE_VALUE,
        .value.value_ptr = (int32_t*)&system_params.display_brightness,
        .value.min = 0,
        .value.max = 100,
        .value.step = 10,
        .value_changed_callback = menu_value_display_brightness
    },
    {
        .name = "Bluetooth",
        .type = MENU_ITEM_TYPE_SWITCH,
        .switch_state = &system_params.bt_enable
    }
};

/* 主菜单项 */
static menu_item_t main_menu_items[] = {
    {
        .name = "Oscilloscope",
        .type = MENU_ITEM_TYPE_ACTION,
        .action_callback = menu_action_oscilloscope
    },
    {
        .name = "Bluetooth",
        .type = MENU_ITEM_TYPE_ACTION,
        .action_callback = menu_action_bluetooth
    },
    {
        .name = "LED Test",
        .type = MENU_ITEM_TYPE_ACTION,
        .action_callback = menu_action_led_test
    },
    {
        .name = "Settings",
        .type = MENU_ITEM_TYPE_SUBMENU,
        .submenu.items = settings_items,
        .submenu.item_count = sizeof(settings_items) / sizeof(settings_items[0])
    }
};

static menu_item_t *main_menu_ptr[] = {
    &main_menu_items[0],
    &main_menu_items[1],
    &main_menu_items[2],
    &main_menu_items[3]
};

/*=============================================================================
 *                              波形显示接口适配
 *============================================================================*/

/* ADC数据源接口 */
static int waveform_adc_init(void)
{
    adc_channel_config_t config = bsp_adc_get_default_config();
    return bsp_adc_init(&config);
}

static void waveform_adc_deinit(void)
{
    bsp_adc_deinit();
}

static uint16_t waveform_adc_read(void)
{
    return bsp_adc_read();
}

static int waveform_adc_read_buffer(uint16_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        buf[i] = bsp_adc_read();
    }
    return 0;
}

static void waveform_adc_set_rate(uint32_t rate)
{
    (void)rate; /* 简化实现 */
}

static const waveform_data_source_t waveform_adc_source = {
    .init = waveform_adc_init,
    .deinit = waveform_adc_deinit,
    .read = waveform_adc_read,
    .read_buffer = waveform_adc_read_buffer,
    .set_sample_rate = waveform_adc_set_rate
};

/* TFT显示接口 */
static void waveform_display_clear(void)
{
    bsp_tft_clear(TFT_BLACK);
}

static void waveform_display_pixel(int16_t x, int16_t y)
{
    bsp_tft_draw_pixel(x, y, TFT_GREEN);
}

static void waveform_display_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    bsp_tft_draw_line(x0, y0, x1, y1, TFT_GREEN);
}

static void waveform_display_hline(int16_t x, int16_t y, int16_t w)
{
    bsp_tft_draw_hline(x, y, w, TFT_GRAY);
}

static void waveform_display_vline(int16_t x, int16_t y, int16_t h)
{
    bsp_tft_draw_vline(x, y, h, TFT_GRAY);
}

static void waveform_display_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    bsp_tft_draw_rect(x, y, w, h, TFT_WHITE);
}

static void waveform_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    bsp_tft_fill_rect(x, y, w, h, TFT_BLACK);
}

static void waveform_display_string(int16_t x, int16_t y, const char *str)
{
    bsp_tft_draw_string(x, y, str, &font_8x16, TFT_WHITE, TFT_BLACK);
}

static void waveform_display_update(void)
{
    /* TFT无需手动刷新 */
}

static void waveform_display_set_color(uint8_t color)
{
    (void)color;
}

static const waveform_display_interface_t waveform_tft_display = {
    .clear = waveform_display_clear,
    .draw_pixel = waveform_display_pixel,
    .draw_line = waveform_display_line,
    .draw_hline = waveform_display_hline,
    .draw_vline = waveform_display_vline,
    .draw_rect = waveform_display_rect,
    .fill_rect = waveform_display_fill_rect,
    .draw_string = waveform_display_string,
    .update = waveform_display_update,
    .set_color = waveform_display_set_color
};

/*=============================================================================
 *                              任务实现
 *============================================================================*/

/**
 * @brief EC11扫描任务 (10ms周期)
 */
static void task_ec11_scan(void *arg)
{
    (void)arg;
    ec11_event_t event = bsp_ec11_scan();
    if (event != EC11_EVENT_NONE) {
        ec11_event_handler(event);
    }
}

/**
 * @brief 按键扫描任务 (20ms周期)
 */
static void task_key_scan(void *arg)
{
    (void)arg;
    bsp_key_scan();
}

/**
 * @brief ADC采样任务 (根据模式运行)
 */
static void task_adc_sample(void *arg)
{
    (void)arg;

    if (current_mode == APP_MODE_OSCILLOSCOPE) {
        /* 波形采样和更新 */
        waveform_update();
    }
}

/**
 * @brief 显示更新任务 (50ms周期)
 */
static void task_display_update(void *arg)
{
    (void)arg;

    switch (current_mode) {
    case APP_MODE_MENU:
        menu_refresh();
        break;

    case APP_MODE_OSCILLOSCOPE:
        /* 波形显示在task_adc_sample中处理 */
        break;

    case APP_MODE_BLUETOOTH:
        /* 显示蓝牙状态 */
        bsp_tft_clear(TFT_BLACK);
        bsp_tft_draw_string(20, 100, "Bluetooth Mode", &font_8x16, TFT_CYAN, TFT_BLACK);
        if (bsp_bluetooth_is_connected()) {
            bsp_tft_draw_string(40, 130, "Connected", &font_8x16, TFT_GREEN, TFT_BLACK);
        } else {
            bsp_tft_draw_string(40, 130, "Waiting...", &font_8x16, TFT_YELLOW, TFT_BLACK);
        }
        break;
    }
}

/**
 * @brief 蓝牙处理任务 (100ms周期)
 */
static void task_bluetooth_process(void *arg)
{
    (void)arg;

    if (system_params.bt_enable) {
        bsp_bluetooth_process();

        /* 示波器模式下定时发送波形数据 */
        if (current_mode == APP_MODE_OSCILLOSCOPE && bsp_bluetooth_is_connected()) {
            /* 发送ADC数据 (简化: 只发当前值) */
            uint16_t adc_val = bsp_adc_read();
            uint8_t data[2] = {adc_val >> 8, adc_val & 0xFF};
            bsp_bluetooth_send_frame(BT_CMD_ADC_DATA, data, 2);
        }
    }
}

/**
 * @brief LED呼吸灯任务 (20ms周期)
 */
static void task_led_breath(void *arg)
{
    (void)arg;
    bsp_pwm_breath_update();
}

/**
 * @brief 系统监控任务 (1000ms周期)
 */
static void task_system_monitor(void *arg)
{
    (void)arg;

    uint32_t hours, minutes, seconds;
    bsp_timer_get_uptime(&hours, &minutes, &seconds);

    DEBUG_PRINT("Uptime: %02lu:%02lu:%02lu, CPU: %.1f%%\r\n",
                hours, minutes, seconds,
                scheduler_get_cpu_usage());
}

/*=============================================================================
 *                              菜单显示回调
 *============================================================================*/

static void menu_display_callback(const menu_state_t *state)
{
    uint8_t i;
    uint16_t y = 20;
    char buf[32];

    bsp_tft_clear(TFT_BLACK);

    /* 标题栏 */
    bsp_tft_fill_rect(0, 0, 240, 18, TFT_BLUE);
    bsp_tft_draw_string(80, 1, "MENU", &font_8x16, TFT_WHITE, TFT_BLUE);

    /* 菜单项 */
    for (i = state->display_start; i < state->display_start + 6 && i < state->item_count; i++) {
        menu_item_t *item = state->current_items[i];
        tft_color_t bg = (i == state->selected_index) ? TFT_DARKGRAY : TFT_BLACK;
        tft_color_t fg = TFT_WHITE;

        /* 背景 */
        if (i == state->selected_index) {
            bsp_tft_fill_rect(0, y, 240, 20, bg);
        }

        /* 名称 */
        bsp_tft_draw_string(5, y + 2, item->name, &font_8x16, fg, bg);

        /* 值显示 */
        switch (item->type) {
        case MENU_ITEM_TYPE_VALUE:
            snprintf(buf, sizeof(buf), "%ld", (long)*item->value.value_ptr);
            bsp_tft_draw_string(180, y + 2, buf, &font_8x16, TFT_YELLOW, bg);
            break;
        case MENU_ITEM_TYPE_SWITCH:
            bsp_tft_draw_string(180, y + 2, *item->switch_state ? "ON" : "OFF",
                               &font_8x16, *item->switch_state ? TFT_GREEN : TFT_RED, bg);
            break;
        case MENU_ITEM_TYPE_SUBMENU:
            bsp_tft_draw_string(210, y + 2, ">", &font_8x16, TFT_CYAN, bg);
            break;
        default:
            break;
        }

        y += 22;
    }

    /* 底部状态栏 */
    bsp_tft_fill_rect(0, 222, 240, 18, TFT_DARKGRAY);
    snprintf(buf, sizeof(buf), "Depth:%d  Item:%d/%d",
             state->depth + 1, state->selected_index + 1, state->item_count);
    bsp_tft_draw_string(5, 223, buf, &font_8x16, TFT_WHITE, TFT_DARKGRAY);
}

/*=============================================================================
 *                              主函数
 *============================================================================*/

int main(void)
{
    /* 板级初始化 */
    board_init();

    /* 定时器初始化 */
    bsp_timer_init();

    /* 调度器初始化 */
    scheduler_init();

    /* TFT初始化 */
    bsp_tft_init();
    bsp_tft_set_brightness(system_params.display_brightness);
    bsp_tft_clear(TFT_BLACK);
    bsp_tft_draw_string(60, 100, "Initializing...", &font_8x16, TFT_WHITE, TFT_BLACK);

    /* 串口初始化 (调试) */
    bsp_uart_init(UART_PORT_1, NULL);
    DEBUG_PRINT("\r\n=== System Starting ===\r\n");

    /* EC11初始化 */
    bsp_ec11_init();

    /* ADC初始化 */
    adc_channel_config_t adc_cfg = bsp_adc_get_default_config();
    bsp_adc_init(&adc_cfg);

    /* PWM初始化 (LED) */
    pwm_config_t pwm_cfg = bsp_pwm_get_preset_config(PWM_TIMER_3, PWM_CH_1);
    led_pwm_channel = bsp_pwm_init(&pwm_cfg);
    bsp_pwm_set_duty_percent(led_pwm_channel, system_params.led_brightness);
    bsp_pwm_start(led_pwm_channel);

    /* 蓝牙初始化 */
    if (system_params.bt_enable) {
        bsp_bluetooth_init();
        bsp_bluetooth_set_frame_callback(bluetooth_frame_handler);
        bsp_bluetooth_set_state_callback(bluetooth_state_handler);
    }

    /* 波形显示模块初始化 */
    waveform_init(&waveform_adc_source, &waveform_tft_display);

    /* 菜单初始化 */
    menu_init(main_menu_ptr, sizeof(main_menu_ptr) / sizeof(main_menu_ptr[0]), menu_display_callback);

    /* 创建任务 */
    task_config_t task_cfg;

    /* EC11扫描 - 高优先级, 10ms */
    task_cfg = (task_config_t)TASK_PERIODIC("EC11", task_ec11_scan, 10, TASK_PRIORITY_HIGH);
    scheduler_task_create(&task_cfg);

    /* 按键扫描 - 普通优先级, 20ms */
    task_cfg = (task_config_t)TASK_PERIODIC("Key", task_key_scan, 20, TASK_PRIORITY_NORMAL);
    scheduler_task_create(&task_cfg);

    /* ADC采样 - 高优先级, 20ms */
    task_cfg = (task_config_t)TASK_PERIODIC("ADC", task_adc_sample, 20, TASK_PRIORITY_HIGH);
    scheduler_task_create(&task_cfg);

    /* 显示更新 - 普通优先级, 50ms */
    task_cfg = (task_config_t)TASK_PERIODIC("Display", task_display_update, 50, TASK_PRIORITY_NORMAL);
    scheduler_task_create(&task_cfg);

    /* 蓝牙处理 - 低优先级, 100ms */
    task_cfg = (task_config_t)TASK_PERIODIC("BT", task_bluetooth_process, 100, TASK_PRIORITY_LOW);
    scheduler_task_create(&task_cfg);

    /* LED呼吸灯 - 低优先级, 20ms */
    task_cfg = (task_config_t)TASK_PERIODIC("LED", task_led_breath, 20, TASK_PRIORITY_LOW);
    scheduler_task_create(&task_cfg);

    /* 系统监控 - 空闲优先级, 1000ms */
    task_cfg = (task_config_t)TASK_PERIODIC("Monitor", task_system_monitor, 1000, TASK_PRIORITY_IDLE);
    scheduler_task_create(&task_cfg);

    DEBUG_PRINT("All tasks created. Starting scheduler...\r\n");

    /* 启动调度器 (不返回) */
    scheduler_start();

    /* 不会执行到这里 */
    while (1);
}

/*=============================================================================
 *                              SysTick中断
 *============================================================================*/

void SysTick_Handler(void)
{
    scheduler_tick();
}
