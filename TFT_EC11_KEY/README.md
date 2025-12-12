# TFT_EC11_KEY - 嵌入式驱动库

## 写在前面 - 给嵌入式萌新的话

如果你只会C语言，完全不了解嵌入式开发，不要担心！这个项目就是为你准备的。

我会用**最通俗的语言**，一步一步教你：
1. 什么是嵌入式？
2. 这个项目是干什么的？
3. 怎么把代码烧录到开发板？
4. 怎么修改代码实现你想要的功能？

---

## 第一章：嵌入式是什么？

### 1.1 简单理解

想象一下：
- 你的电脑是一个"大脑"，可以运行各种程序
- **单片机**也是一个"大脑"，但它很小，专门用来控制一些小设备

比如：
- 电饭煲里有单片机 → 控制加热、保温
- 空调遥控器里有单片机 → 检测按键、发送红外信号
- 你的开发板就是一个单片机

### 1.2 我们用的单片机

```
┌──────────────────────────────────────┐
│     STM32F407VGT6 单片机             │
│                                      │
│  - 主频: 168MHz (很快！)             │
│  - 内存: 192KB (存放变量)            │
│  - 闪存: 1MB (存放程序)              │
│  - 引脚: 很多GPIO可以连接外设         │
│                                      │
│  简单说：这是一个很强大的单片机       │
└──────────────────────────────────────┘
```

### 1.3 什么是GPIO？

GPIO = General Purpose Input Output = 通用输入输出

简单说就是单片机上的"引脚"，你可以：
- 让引脚**输出**高电平(3.3V)或低电平(0V) → 控制LED亮灭
- **读取**引脚的电平 → 检测按键是否按下

---

## 第二章：这个项目是干什么的？

### 2.1 项目功能一览

```
┌─────────────────────────────────────────────────────────┐
│                    TFT_EC11_KEY 项目                     │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐             │
│  │ EC11    │    │  TFT    │    │ 蓝牙    │             │
│  │ 旋钮    │    │ 彩屏    │    │ 模块    │             │
│  └────┬────┘    └────┬────┘    └────┬────┘             │
│       │              │              │                   │
│       ▼              ▼              ▼                   │
│  ┌─────────────────────────────────────────────────┐   │
│  │              STM32F407 单片机                    │   │
│  │                                                  │   │
│  │  运行"调度器"管理所有任务：                       │   │
│  │  - 每10ms检测旋钮                                │   │
│  │  - 每50ms更新显示                                │   │
│  │  - 每100ms处理蓝牙                               │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 2.2 你可以用这个项目做什么？

1. **旋钮菜单系统** - 转动旋钮选择菜单，按下确认
2. **简易示波器** - 采集电压波形并显示
3. **蓝牙控制** - 手机通过蓝牙控制开发板
4. **呼吸灯效果** - PWM控制LED亮度渐变
5. **更多功能** - 你可以在此基础上扩展！

---

## 第三章：项目结构（超详细解释）

### 3.1 文件夹结构

```
TFT_EC11_KEY/
│
├── app/                    ← 【应用层】你写的主程序放这里
│   ├── main.c              ← 最简单的示例
│   ├── main_app.c          ← 完整功能示例（推荐看这个）
│   └── main_example.c      ← 菜单系统示例
│
├── board/                  ← 【板级】开发板相关的初始化
│   └── board.c/h           ← 时钟配置、延时函数
│
├── bsp/                    ← 【BSP驱动层】各种硬件驱动
│   ├── bsp_ec11.c/h        ← EC11旋转编码器驱动
│   ├── bsp_tft_st7789.c/h  ← TFT彩屏驱动
│   ├── bsp_oled_ssd1306.c/h← OLED屏驱动（0.96寸/0.91寸）
│   ├── bsp_adc.c/h         ← ADC采样驱动
│   ├── bsp_uart.c/h        ← 串口驱动
│   ├── bsp_bluetooth.c/h   ← 蓝牙模块驱动
│   ├── bsp_pwm.c/h         ← PWM输出驱动
│   └── bsp_timer.c/h       ← 定时器驱动
│
├── bsp_hal/                ← 【HAL库版本】用STM32CubeMX的看这里
│   ├── bsp_adc_hal.c/h     ← ADC HAL版
│   ├── bsp_tft_hal.c/h     ← TFT HAL版
│   ├── bsp_uart_hal.c/h    ← UART HAL版
│   └── bsp_oled_hal.c/h    ← OLED HAL版
│
├── middleware/             ← 【中间件】高级功能模块
│   ├── scheduler.c/h       ← ★重要★ 任务调度器
│   ├── menu_core.c/h       ← 菜单系统核心
│   └── waveform_display.c/h← 波形显示模块
│
├── doc/                    ← 【文档】各种手册
│   └── API参考手册.md
│
├── libraries/              ← 标准库文件（不用管）
├── module/                 ← 中断配置（不用管）
└── project/                ← Keil工程文件
```

### 3.2 三层架构解释

```
你写的代码 (main_app.c)
        │
        ▼
┌───────────────────────────────────────────┐
│           应用层 (Application)             │  ← 你在这里写代码
│  - 实现具体功能逻辑                        │
│  - 调用中间件提供的功能                    │
└───────────────────┬───────────────────────┘
                    │
                    ▼
┌───────────────────────────────────────────┐
│           中间件 (Middleware)              │  ← 封装好的功能模块
│  - 任务调度器：管理多个任务                 │
│  - 菜单系统：处理菜单逻辑                  │
│  - 波形显示：示波器功能                    │
│                                           │
│  特点：与硬件无关，可以直接用               │
└───────────────────┬───────────────────────┘
                    │
                    ▼
┌───────────────────────────────────────────┐
│           BSP驱动层 (BSP)                  │  ← 硬件驱动
│  - 直接操作硬件寄存器                      │
│  - 不同芯片需要修改这层                    │
│                                           │
│  如果你换了芯片，主要改这层                 │
└───────────────────────────────────────────┘
```

---

## 第四章：快速开始（手把手教程）

### 4.1 你需要准备的东西

**硬件：**
- 立创梁山派天空星开发板（STM32F407）
- EC11旋转编码器模块
- TFT彩屏（ST7789）或OLED屏（SSD1306）
- USB数据线
- 杜邦线若干

**软件：**
- Keil MDK（编译代码用）
- ST-Link驱动（烧录程序用）

### 4.2 硬件连接

#### EC11旋转编码器连接：

```
EC11模块          开发板
────────          ──────
  A相    ────────   PA0    (旋转检测)
  B相    ────────   PA1    (旋转检测)
  按键   ────────   PA2    (按下检测)
  VCC    ────────   3.3V
  GND    ────────   GND
```

#### TFT彩屏连接（SPI接口）：

```
TFT屏幕           开发板
───────           ──────
  SCL    ────────   PB3    (SPI时钟)
  SDA    ────────   PB5    (SPI数据)
  CS     ────────   PB4    (片选)
  DC     ────────   PB7    (数据/命令)
  RES    ────────   PB6    (复位)
  BLK    ────────   PB8    (背光)
  VCC    ────────   3.3V
  GND    ────────   GND
```

#### OLED屏连接（I2C接口）：

```
OLED屏幕          开发板
────────          ──────
  SCL    ────────   PB6    (I2C时钟)
  SDA    ────────   PB7    (I2C数据)
  VCC    ────────   3.3V
  GND    ────────   GND
```

### 4.3 编译和烧录

1. 打开 `project/xxx.uvprojx` 工程文件
2. 点击"Build"按钮（或按F7）编译
3. 编译成功后，点击"Download"按钮烧录
4. 按下开发板的复位键，程序开始运行！

---

## 第五章：代码详解（萌新必看）

### 5.1 最简单的例子 - 点亮LED

```c
#include "stm32f4xx.h"

int main(void)
{
    // 第1步：开启GPIOA的时钟（单片机省电设计，用之前要开时钟）
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // 第2步：配置PA0为输出模式
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;      // 选择PA0引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  // 输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 第3步：控制LED
    while (1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_0);    // PA0输出高电平，LED亮
        delay_ms(500);                       // 延时500毫秒
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);  // PA0输出低电平，LED灭
        delay_ms(500);                       // 延时500毫秒
    }
}
```

### 5.2 使用调度器的完整例子

```c
#include "board.h"
#include "middleware/scheduler.h"
#include "bsp/bsp_ec11.h"
#include "bsp/bsp_tft_st7789.h"

// ============ 任务函数 ============
// 任务就是一个普通的函数，调度器会定时调用它

// 任务1：检测旋钮（每10毫秒执行一次）
void task_ec11(void *arg)
{
    ec11_event_t event = bsp_ec11_scan();  // 检测旋钮

    if (event == EC11_EVENT_ROTATE_LEFT) {
        // 旋钮向左转了！
        // 在这里写你想做的事情
    }
    else if (event == EC11_EVENT_ROTATE_RIGHT) {
        // 旋钮向右转了！
    }
    else if (event == EC11_EVENT_KEY_SHORT_PRESS) {
        // 旋钮被短按了！
    }
}

// 任务2：更新显示（每50毫秒执行一次）
void task_display(void *arg)
{
    // 在屏幕上显示一些内容
    bsp_tft_draw_string(10, 10, "Hello!", &font_8x16, TFT_WHITE, TFT_BLACK);
}

// ============ 主函数 ============
int main(void)
{
    // 1. 初始化开发板
    board_init();

    // 2. 初始化调度器
    scheduler_init();

    // 3. 初始化硬件
    bsp_ec11_init();
    bsp_tft_init();

    // 4. 创建任务
    task_config_t cfg;

    // 创建EC11任务：每10ms执行一次，高优先级
    cfg = (task_config_t)TASK_PERIODIC("EC11", task_ec11, 10, TASK_PRIORITY_HIGH);
    scheduler_task_create(&cfg);

    // 创建显示任务：每50ms执行一次，普通优先级
    cfg = (task_config_t)TASK_PERIODIC("Display", task_display, 50, TASK_PRIORITY_NORMAL);
    scheduler_task_create(&cfg);

    // 5. 启动调度器（程序从这里开始循环执行任务）
    scheduler_start();  // 这个函数不会返回！

    // 下面的代码永远不会执行
    while (1);
}
```

### 5.3 理解"调度器"

调度器就像一个**任务管理员**：

```
时间线：
0ms   10ms  20ms  30ms  40ms  50ms  60ms  70ms  80ms  90ms  100ms
 │     │     │     │     │     │     │     │     │     │      │
 │     │     │     │     │     │     │     │     │     │      │
 ▼     ▼     ▼     ▼     ▼     ▼     ▼     ▼     ▼     ▼      ▼
EC11  EC11  EC11  EC11  EC11  EC11  EC11  EC11  EC11  EC11   EC11
             │           │           │           │            │
             ▼           ▼           ▼           ▼            ▼
          Display     Display     Display     Display      Display

解释：
- EC11任务每10ms执行一次 → 响应快，旋钮手感好
- Display任务每50ms执行一次 → 刷新率20帧/秒，足够流畅
```

**为什么要用调度器？**

不用调度器的写法（不推荐）：
```c
while (1) {
    bsp_ec11_scan();      // 检测旋钮
    update_display();     // 更新显示
    process_bluetooth();  // 处理蓝牙
    delay_ms(10);         // 延时
}
// 问题：所有任务执行频率一样，不灵活
```

用调度器的写法（推荐）：
```c
// 每个任务独立，执行频率可以不同！
scheduler_task_create("EC11", task_ec11, 10ms, HIGH);      // 10ms一次
scheduler_task_create("Display", task_display, 50ms, NORMAL); // 50ms一次
scheduler_task_create("BT", task_bt, 100ms, LOW);          // 100ms一次
```

---

## 第六章：常用API速查表

### 6.1 EC11旋转编码器

```c
// 初始化
bsp_ec11_init();

// 检测事件（需要周期性调用，建议10ms）
ec11_event_t event = bsp_ec11_scan();

// 事件类型
EC11_EVENT_NONE           // 无事件
EC11_EVENT_ROTATE_LEFT    // 左旋（逆时针）
EC11_EVENT_ROTATE_RIGHT   // 右旋（顺时针）
EC11_EVENT_KEY_SHORT_PRESS // 短按
EC11_EVENT_KEY_LONG_PRESS  // 长按
```

### 6.2 TFT彩屏

```c
// 初始化
bsp_tft_init();

// 清屏
bsp_tft_clear(TFT_BLACK);  // 用黑色清屏

// 画点
bsp_tft_draw_pixel(x, y, TFT_WHITE);

// 画线
bsp_tft_draw_line(x0, y0, x1, y1, TFT_RED);

// 画矩形
bsp_tft_draw_rect(x, y, width, height, TFT_GREEN);
bsp_tft_fill_rect(x, y, width, height, TFT_BLUE);  // 填充

// 显示文字
bsp_tft_draw_string(x, y, "Hello", &font_8x16, TFT_WHITE, TFT_BLACK);

// 常用颜色
TFT_BLACK, TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE
TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_ORANGE
```

### 6.3 OLED屏

```c
// 初始化
bsp_oled_init();

// 清屏
bsp_oled_clear();

// 刷新显示（OLED有显存，改完要刷新才能看到）
bsp_oled_refresh();

// 画点
bsp_oled_draw_pixel(x, y, OLED_COLOR_WHITE);

// 显示文字
bsp_oled_draw_string(x, y, "Hello", &oled_font_6x8, OLED_COLOR_WHITE);

// 显示数字
bsp_oled_draw_num(x, y, 12345, &oled_font_6x8, OLED_COLOR_WHITE);
```

### 6.4 ADC采样

```c
// 初始化
adc_channel_config_t cfg = bsp_adc_get_default_config();
bsp_adc_init(&cfg);

// 读取原始值（0-4095）
uint16_t raw = bsp_adc_read();

// 读取电压（毫伏）
uint32_t voltage_mv = bsp_adc_read_voltage();
// 比如返回1650，表示1.65V
```

### 6.5 蓝牙

```c
// 初始化
bsp_bluetooth_init();

// 发送数据
bsp_bluetooth_send("Hello", 5);
bsp_bluetooth_printf("Temperature: %d\r\n", temp);

// 设置回调函数（收到数据时自动调用）
void my_callback(const bt_frame_t *frame) {
    // 处理收到的数据
}
bsp_bluetooth_set_frame_callback(my_callback);

// 周期处理（放在任务中）
bsp_bluetooth_process();
```

### 6.6 调度器

```c
// 初始化
scheduler_init();

// 创建周期任务
task_config_t cfg = TASK_PERIODIC("TaskName", task_func, period_ms, priority);
scheduler_task_create(&cfg);

// 优先级（从低到高）
TASK_PRIORITY_IDLE     // 空闲时才执行
TASK_PRIORITY_LOW      // 低优先级
TASK_PRIORITY_NORMAL   // 普通（默认）
TASK_PRIORITY_HIGH     // 高优先级
TASK_PRIORITY_REALTIME // 实时（最高）

// 启动调度器
scheduler_start();  // 不会返回！
```

---

## 第七章：如何修改代码实现自己的功能

### 7.1 例子：用旋钮控制LED亮度

```c
#include "board.h"
#include "middleware/scheduler.h"
#include "bsp/bsp_ec11.h"
#include "bsp/bsp_pwm.h"
#include "bsp/bsp_tft_st7789.h"

// 全局变量：LED亮度（0-100）
static uint8_t led_brightness = 50;
static pwm_channel_t led_channel;

// 任务：检测旋钮
void task_ec11(void *arg)
{
    ec11_event_t event = bsp_ec11_scan();

    if (event == EC11_EVENT_ROTATE_LEFT) {
        // 左转：减小亮度
        if (led_brightness >= 10) {
            led_brightness -= 10;
        }
        bsp_pwm_set_duty_percent(led_channel, led_brightness);
    }
    else if (event == EC11_EVENT_ROTATE_RIGHT) {
        // 右转：增加亮度
        if (led_brightness <= 90) {
            led_brightness += 10;
        }
        bsp_pwm_set_duty_percent(led_channel, led_brightness);
    }
}

// 任务：更新显示
void task_display(void *arg)
{
    char buf[32];

    bsp_tft_fill_rect(0, 0, 240, 30, TFT_BLACK);  // 清除上一次的文字
    snprintf(buf, sizeof(buf), "Brightness: %d%%", led_brightness);
    bsp_tft_draw_string(10, 10, buf, &font_8x16, TFT_WHITE, TFT_BLACK);
}

int main(void)
{
    board_init();
    scheduler_init();

    // 初始化硬件
    bsp_ec11_init();
    bsp_tft_init();

    // 初始化PWM（用于控制LED）
    pwm_config_t pwm_cfg = bsp_pwm_get_preset_config(PWM_TIMER_3, PWM_CH_1);
    led_channel = bsp_pwm_init(&pwm_cfg);
    bsp_pwm_set_duty_percent(led_channel, led_brightness);
    bsp_pwm_start(led_channel);

    // 创建任务
    task_config_t cfg;
    cfg = (task_config_t)TASK_PERIODIC("EC11", task_ec11, 10, TASK_PRIORITY_HIGH);
    scheduler_task_create(&cfg);

    cfg = (task_config_t)TASK_PERIODIC("Display", task_display, 100, TASK_PRIORITY_NORMAL);
    scheduler_task_create(&cfg);

    scheduler_start();
    while (1);
}
```

### 7.2 例子：用蓝牙控制LED

```c
// 蓝牙接收回调
void bluetooth_callback(const bt_frame_t *frame)
{
    if (frame->cmd == 0x10) {  // 假设0x10是控制LED的命令
        led_brightness = frame->data[0];  // 第一个字节是亮度值
        bsp_pwm_set_duty_percent(led_channel, led_brightness);
    }
}

// 在main中设置回调
bsp_bluetooth_set_frame_callback(bluetooth_callback);
```

---

## 第八章：常见问题解答（FAQ）

### Q1: 编译报错"找不到头文件"怎么办？

**A:** 检查Keil的Include路径设置，确保包含：
- `bsp/`
- `middleware/`
- `board/`
- `bsp_hal/`

### Q2: 烧录后没反应怎么办？

**A:**
1. 检查USB线是否连接好
2. 检查ST-Link驱动是否安装
3. 按下复位键试试
4. 检查电源是否正常

### Q3: 屏幕不亮怎么办？

**A:**
1. 检查接线是否正确
2. 检查电源是否为3.3V
3. 检查SPI/I2C引脚配置
4. 用万用表测量背光引脚电压

### Q4: 旋钮方向反了怎么办？

**A:** 交换A相和B相的接线

### Q5: 标准库版和HAL库版有什么区别？

**A:**
- **标准库版**：直接操作寄存器，效率高，代码更底层
- **HAL库版**：使用ST官方HAL库，更容易理解，适合配合CubeMX使用

如果你用CubeMX配置外设，用HAL库版更方便。
如果追求执行效率，用标准库版。

---

## 第九章：版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 2.0.0 | 2025-12-12 | 添加调度器、TFT、蓝牙、OLED等功能 |
| 1.0.0 | 2024-03-07 | 初始版本，EC11+菜单系统 |

---

## 第十章：获取帮助

如果遇到问题：
1. 先看本文档的FAQ部分
2. 查看 `doc/API参考手册.md` 了解详细接口
3. 在GitHub Issues中提问

---

**祝你嵌入式学习顺利！**

**Author**: Claude Code
**Platform**: STM32F407VGT6 (立创梁山派天空星)
