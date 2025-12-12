# API参考手册

## STM32F407 嵌入式驱动库 v2.0

**版本**: 2.0.0
**日期**: 2025-12-12
**作者**: Claude Code

---

## 目录

1. [架构概述](#架构概述)
2. [BSP驱动层](#bsp驱动层)
   - [ADC驱动](#adc驱动-bsp_adch)
   - [EC11编码器](#ec11编码器-bsp_ec11h)
   - [按键驱动](#按键驱动-bsp_keyh)
   - [TFT显示驱动](#tft显示驱动-bsp_tft_st7789h)
   - [UART串口驱动](#uart串口驱动-bsp_uarth)
   - [蓝牙驱动](#蓝牙驱动-bsp_bluetoothh)
   - [PWM驱动](#pwm驱动-bsp_pwmh)
   - [定时器驱动](#定时器驱动-bsp_timerh)
3. [中间件层](#中间件层)
   - [调度器](#调度器-schedulerh)
   - [菜单系统](#菜单系统-menu_coreh)
   - [波形显示](#波形显示-waveform_displayh)
   - [配置管理](#配置管理-menu_configh)
   - [动画效果](#动画效果-menu_animationh)
4. [HAL库版本](#hal库版本)
5. [移植指南](#移植指南)
6. [常见问题](#常见问题)

---

## 架构概述

```
┌────────────────────────────────────────┐
│            Application Layer            │
│     (main_app.c, user application)      │
└───────────────────┬────────────────────┘
                    │
┌───────────────────▼────────────────────┐
│           Middleware Layer              │
│  scheduler | menu_core | waveform      │
│  menu_config | menu_animation          │
└───────────────────┬────────────────────┘
                    │
┌───────────────────▼────────────────────┐
│              BSP Layer                  │
│  adc | ec11 | tft | uart | bluetooth   │
│  pwm | timer | key | u8g2_port         │
└───────────────────┬────────────────────┘
                    │
┌───────────────────▼────────────────────┐
│           Hardware Layer                │
│     STM32F407 Standard Peripheral       │
└────────────────────────────────────────┘
```

---

## BSP驱动层

### ADC驱动 (bsp_adc.h)

#### 功能特性
- 12位ADC，0-3.3V量程
- 单通道/多通道支持
- DMA连续采样模式
- 内部温度传感器读取
- 滤波功能

#### 核心API

```c
// 初始化
int bsp_adc_init(const adc_channel_config_t *channel);
int bsp_adc_init_dma(const adc_channel_config_t *channel,
                     uint16_t *buffer, uint32_t buffer_size,
                     adc_complete_callback_t callback);

// 读取
uint16_t bsp_adc_read(void);                    // 单次采样
uint16_t bsp_adc_read_average(uint8_t times);   // 均值滤波
uint32_t bsp_adc_read_voltage(void);            // 读取电压(mV)
int16_t bsp_adc_read_temperature(void);         // 读取温度(0.1℃)

// 转换
uint32_t bsp_adc_to_voltage(uint16_t raw_value); // ADC值转电压
```

#### 使用示例

```c
// 初始化ADC
adc_channel_config_t config = bsp_adc_get_default_config();
bsp_adc_init(&config);

// 读取电压
uint32_t voltage_mv = bsp_adc_read_voltage();
printf("Voltage: %lu mV\n", voltage_mv);

// 读取温度
int16_t temp = bsp_adc_read_temperature();
printf("Temperature: %.1f C\n", temp / 10.0f);
```

---

### EC11编码器 (bsp_ec11.h)

#### 事件类型

| 事件 | 说明 |
|------|------|
| EC11_EVENT_NONE | 无事件 |
| EC11_EVENT_ROTATE_LEFT | 左旋 |
| EC11_EVENT_ROTATE_RIGHT | 右旋 |
| EC11_EVENT_KEY_SHORT_PRESS | 短按 |
| EC11_EVENT_KEY_LONG_PRESS | 长按 |
| EC11_EVENT_KEY_RELEASE | 释放 |

#### 核心API

```c
void bsp_ec11_init(void);
ec11_event_t bsp_ec11_scan(void);       // 需周期调用(10ms)
void bsp_ec11_register_callback(ec11_callback_t callback);
int32_t bsp_ec11_get_count(void);       // 获取累计计数
void bsp_ec11_set_count(int32_t count); // 设置计数值
```

#### 使用示例

```c
bsp_ec11_init();
bsp_ec11_register_callback(ec11_handler);

// 在定时器或主循环中
ec11_event_t event = bsp_ec11_scan();
switch (event) {
    case EC11_EVENT_ROTATE_LEFT:
        menu_move_up();
        break;
    case EC11_EVENT_ROTATE_RIGHT:
        menu_move_down();
        break;
    // ...
}
```

---

### TFT显示驱动 (bsp_tft_st7789.h)

#### 支持分辨率
- 240x240
- 240x320
- 135x240

#### 颜色格式
RGB565 (16位)

#### 预定义颜色

```c
TFT_BLACK, TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE
TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE
TFT_GRAY, TFT_DARKGRAY, TFT_LIGHTGRAY
```

#### 核心API

```c
// 初始化
int bsp_tft_init(void);
void bsp_tft_set_rotation(uint8_t rotation);

// 基本绘图
void bsp_tft_clear(tft_color_t color);
void bsp_tft_draw_pixel(uint16_t x, uint16_t y, tft_color_t color);
void bsp_tft_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, tft_color_t color);
void bsp_tft_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color);
void bsp_tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color);
void bsp_tft_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, tft_color_t color);
void bsp_tft_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, tft_color_t color);

// 文字
void bsp_tft_draw_string(uint16_t x, uint16_t y, const char *str,
                          const tft_font_t *font, tft_color_t fg, tft_color_t bg);
void bsp_tft_printf(uint16_t x, uint16_t y, const tft_font_t *font,
                    tft_color_t fg, tft_color_t bg, const char *fmt, ...);

// 图像
void bsp_tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

// 颜色转换
tft_color_t bsp_tft_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);
tft_color_t bsp_tft_hsv_to_rgb565(uint16_t h, uint8_t s, uint8_t v);
```

---

### UART串口驱动 (bsp_uart.h)

#### 支持端口
- UART_PORT_1: USART1 (PA9/PA10) - 调试用
- UART_PORT_2: USART2 (PA2/PA3) - 蓝牙用

#### 核心API

```c
int bsp_uart_init(uart_port_t port, const uart_config_t *config);
void bsp_uart_send_byte(uart_port_t port, uint8_t data);
uint16_t bsp_uart_send(uart_port_t port, const uint8_t *data, uint16_t len);
void bsp_uart_send_string(uart_port_t port, const char *str);
void bsp_uart_printf(uart_port_t port, const char *fmt, ...);

int bsp_uart_receive_byte(uart_port_t port, uint8_t *data);
uint16_t bsp_uart_receive(uart_port_t port, uint8_t *data, uint16_t max_len);
uint16_t bsp_uart_get_rx_count(uart_port_t port);

void bsp_uart_set_rx_callback(uart_port_t port, uart_rx_callback_t callback);
```

#### 快捷宏

```c
DEBUG_PRINT(fmt, ...)   // 调试串口打印
BT_SEND(data, len)      // 蓝牙串口发送
```

---

### 蓝牙驱动 (bsp_bluetooth.h)

#### 支持模块
- HC-05 (SPP蓝牙)
- HC-06 (SPP蓝牙)
- HM-10 (BLE蓝牙)

#### 数据帧格式

```
| Header(0xAA) | CMD | LEN | DATA[0..LEN-1] | Checksum | Tail(0x55) |
```

#### 预定义命令码

```c
BT_CMD_HEARTBEAT    0x00   // 心跳
BT_CMD_ACK          0x01   // 应答
BT_CMD_ADC_DATA     0x10   // ADC数据
BT_CMD_SET_PARAM    0x20   // 设置参数
BT_CMD_GET_PARAM    0x21   // 获取参数
```

#### 核心API

```c
int bsp_bluetooth_init(void);
void bsp_bluetooth_process(void);           // 周期调用
bt_state_t bsp_bluetooth_get_state(void);
uint8_t bsp_bluetooth_is_connected(void);

// 透传模式
uint16_t bsp_bluetooth_send(const uint8_t *data, uint16_t len);
void bsp_bluetooth_printf(const char *fmt, ...);

// 帧模式
int bsp_bluetooth_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len);

// 回调
void bsp_bluetooth_set_rx_callback(bt_rx_callback_t callback);
void bsp_bluetooth_set_frame_callback(bt_frame_callback_t callback);
void bsp_bluetooth_set_state_callback(bt_state_callback_t callback);

// AT命令
int bsp_bluetooth_enter_at_mode(void);
int bsp_bluetooth_set_name(const char *name);
int bsp_bluetooth_set_pin(const char *pin);
int bsp_bluetooth_set_baudrate(uint32_t baudrate);
```

---

### PWM驱动 (bsp_pwm.h)

#### 核心API

```c
pwm_channel_t bsp_pwm_init(const pwm_config_t *config);
void bsp_pwm_set_duty(pwm_channel_t channel, uint16_t duty);
void bsp_pwm_set_duty_percent(pwm_channel_t channel, float percent);
void bsp_pwm_set_frequency(pwm_channel_t channel, uint32_t freq);
void bsp_pwm_start(pwm_channel_t channel);
void bsp_pwm_stop(pwm_channel_t channel);

// 呼吸灯效果
void bsp_pwm_breath_start(pwm_channel_t channel, const pwm_breath_param_t *param);
void bsp_pwm_breath_stop(pwm_channel_t channel);
void bsp_pwm_breath_update(void);  // 主循环调用
```

#### 快捷初始化

```c
// TIM3 CH1 (PA6)
pwm_channel_t ch = PWM_INIT_TIM3_CH1(1000);  // 1kHz

// 设置50%占空比
bsp_pwm_set_duty_percent(ch, 50.0f);
```

---

### 定时器驱动 (bsp_timer.h)

#### 核心API

```c
void bsp_timer_init(void);

// 时间戳
uint32_t bsp_timer_get_ms(void);
uint32_t bsp_timer_get_us(void);

// 延时
void bsp_timer_delay_us(uint32_t us);
void bsp_timer_delay_ms(uint32_t ms);

// 周期定时器
int bsp_timer_start_periodic(uint32_t period_us, timer_callback_t callback);
void bsp_timer_stop_periodic(void);

// 单次定时器
int bsp_timer_start_oneshot(timer_channel_t channel, uint32_t delay_ms, timer_callback_t callback);
void bsp_timer_cancel_oneshot(timer_channel_t channel);

// 超时检测
int bsp_timer_timeout_start(uint8_t id, uint32_t timeout_ms, timeout_callback_t callback);
void bsp_timer_timeout_feed(uint8_t id);

// 运行时间
uint32_t bsp_timer_get_uptime_sec(void);
void bsp_timer_get_uptime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds);
```

---

## 中间件层

### 调度器 (scheduler.h)

#### 任务优先级

| 优先级 | 说明 |
|--------|------|
| TASK_PRIORITY_IDLE | 空闲 (最低) |
| TASK_PRIORITY_LOW | 低 |
| TASK_PRIORITY_NORMAL | 普通 |
| TASK_PRIORITY_HIGH | 高 |
| TASK_PRIORITY_REALTIME | 实时 (最高) |

#### 核心API

```c
// 调度器控制
int scheduler_init(void);
void scheduler_start(void);    // 进入主循环，不返回
void scheduler_run(void);      // 单次调度
void scheduler_tick(void);     // 时基更新(SysTick中调用)

// 任务管理
task_id_t scheduler_task_create(const task_config_t *config);
int scheduler_task_delete(task_id_t task_id);
int scheduler_task_suspend(task_id_t task_id);
int scheduler_task_resume(task_id_t task_id);
int scheduler_task_set_period(task_id_t task_id, uint32_t period_ms);

// 软件定时器
timer_id_t scheduler_timer_create(uint32_t period_ms,
                                   timer_callback_t callback,
                                   void *arg, uint8_t is_periodic);
int scheduler_timer_start(timer_id_t timer_id);
int scheduler_timer_stop(timer_id_t timer_id);

// 统计
float scheduler_get_cpu_usage(void);
void scheduler_print_tasks(void (*print_func)(const char *));
```

#### 快捷宏

```c
// 周期任务
TASK_PERIODIC("TaskName", task_func, period_ms, priority)

// 一次性任务
TASK_ONESHOT("TaskName", task_func, delay_ms, priority)

// 带参数的周期任务
TASK_PERIODIC_ARG("TaskName", task_func, arg, period_ms, priority)
```

#### 使用示例

```c
// 任务函数
void my_task(void *arg) {
    // 任务逻辑
}

// 创建任务
task_config_t cfg = TASK_PERIODIC("MyTask", my_task, 100, TASK_PRIORITY_NORMAL);
scheduler_task_create(&cfg);

// 启动调度器
scheduler_start();
```

---

### 菜单系统 (menu_core.h)

#### 菜单项类型

| 类型 | 说明 |
|------|------|
| MENU_ITEM_TYPE_ACTION | 执行动作 |
| MENU_ITEM_TYPE_SUBMENU | 子菜单 |
| MENU_ITEM_TYPE_VALUE | 数值调节 |
| MENU_ITEM_TYPE_SWITCH | 开关切换 |

#### 核心API

```c
void menu_init(menu_item_t **root_items, uint8_t root_count,
               menu_display_callback_t display_callback);
void menu_move_up(void);
void menu_move_down(void);
void menu_enter(void);
void menu_back(void);
void menu_value_increase(void);
void menu_value_decrease(void);
void menu_refresh(void);
```

---

### 波形显示 (waveform_display.h)

#### 核心API

```c
int waveform_init(const waveform_data_source_t *data_source,
                  const waveform_display_interface_t *display);
void waveform_start(void);
void waveform_stop(void);
void waveform_update(void);

// 设置
void waveform_set_timebase(waveform_timebase_t timebase);
void waveform_set_voltage_div(waveform_voltage_div_t div);
void waveform_set_trigger_mode(waveform_trigger_mode_t mode);
void waveform_set_trigger_level(uint16_t level_mv);
void waveform_auto_setup(void);

// 控制
void waveform_timebase_increase(void);
void waveform_timebase_decrease(void);
void waveform_toggle_grid(void);
void waveform_toggle_measurement(void);
```

---

## HAL库版本

HAL库版本位于 `bsp_hal/` 目录，提供与STM32CubeMX兼容的接口。

### 使用方法

1. 在CubeMX中配置外设
2. 包含对应的HAL头文件
3. 定义 `USE_HAL_DRIVER` 宏
4. 传入HAL句柄调用函数

```c
#define USE_HAL_DRIVER
#include "bsp_hal/bsp_adc_hal.h"

// 使用CubeMX生成的句柄
extern ADC_HandleTypeDef hadc1;
bsp_adc_hal_init(&hadc1);
uint16_t value = bsp_adc_hal_read(&hadc1);
```

---

## 移植指南

### 移植到其他STM32系列

1. **修改头文件包含**
   ```c
   // stm32f4xx.h -> stm32f1xx.h / stm32h7xx.h
   ```

2. **修改引脚配置**
   - 修改各模块头文件中的GPIO定义
   - 确保时钟配置正确

3. **修改时钟频率**
   ```c
   // bsp_timer.c
   #define TIMER_SYSCLK_MHZ    72  // 根据实际修改
   ```

4. **实现弱函数**
   ```c
   // 延时函数
   void delay_ms(uint32_t ms);
   void delay_us(uint32_t us);

   // 时间戳
   uint32_t scheduler_get_us(void);
   ```

### 移植到其他平台

中间件层与硬件无关，只需实现BSP层接口即可。

---

## 常见问题

### Q: 编译报错找不到头文件?
A: 确保Include路径包含 `bsp/`, `middleware/`, `board/` 目录。

### Q: ADC读取值不准确?
A:
- 检查参考电压是否为3.3V
- 使用 `bsp_adc_read_average()` 进行滤波
- 增加采样时间

### Q: EC11旋转方向相反?
A: 交换A、B相引脚连接。

### Q: 蓝牙无法连接?
A:
- 确认波特率匹配(默认9600)
- 检查STATE引脚状态
- 尝试进入AT模式重新配置

### Q: 调度器CPU占用率过高?
A:
- 减少任务执行频率
- 检查任务是否阻塞
- 优化任务代码

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 2.0.0 | 2025-12-12 | 添加调度器、TFT、蓝牙、PWM等模块 |
| 1.0.0 | 2024-03-07 | 初始版本，EC11+菜单系统 |

---

**文档结束**
