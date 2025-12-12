# TFT_EC11_KEY - 嵌入式驱动库 v2.0

高内聚低耦合的嵌入式驱动库 for STM32F407

## 特性

### 核心功能
- **EC11旋转编码器驱动** - 左旋/右旋/短按/长按检测
- **独立按键驱动** - 支持最多4个按键
- **完整菜单系统** - 多级菜单/数值调节/开关切换
- **伪调度器** - 协作式多任务调度，所有应用运行在调度器中

### 显示驱动
- **TFT ST7789** - 240x240/240x320 彩色TFT显示
- **U8G2适配层** - 支持SSD1306/ST7920等OLED

### 通信接口
- **UART串口** - 中断收发/环形缓冲区/printf重定向
- **蓝牙模块** - HC-05/HC-06/HM-10，AT命令/透传/帧协议

### 外设驱动
- **ADC采样** - 单通道/多通道/DMA/波形采集
- **PWM输出** - 多通道/呼吸灯效果
- **定时器** - 微秒延时/时间戳/周期定时

### 高级功能
- **波形显示** - 简易示波器功能
- **EEPROM配置** - 参数自动保存
- **动画效果** - 滑动/淡入淡出/缩放
- **动态菜单** - 运行时创建/删除菜单项

### 架构特点
- 模块化三层架构 (BSP/Middleware/App)
- 同时提供标准库和HAL库版本
- 高内聚低耦合，易于移植

## 硬件平台

- **MCU**: STM32F407VGT6
- **开发板**: 立创梁山派天空星开发板

## 硬件连接

### EC11编码器
| 引脚 | GPIO | 说明 |
|------|------|------|
| A相 | PA0 | EXTI中断 |
| B相 | PA1 | 普通输入 |
| 按键 | PA2 | EXTI中断 |

### TFT显示 (ST7789 SPI)
| 引脚 | GPIO | 说明 |
|------|------|------|
| SCL | PB3 | SPI1_SCK |
| SDA | PB5 | SPI1_MOSI |
| CS | PB4 | 片选 |
| DC | PB7 | 数据/命令 |
| RES | PB6 | 复位 |
| BLK | PB8 | 背光 |

### ADC采样
| 通道 | GPIO | 说明 |
|------|------|------|
| CH4 | PA4 | 默认采样通道 |

### 蓝牙模块 (USART2)
| 引脚 | GPIO | 说明 |
|------|------|------|
| TX | PA2 | 发送 |
| RX | PA3 | 接收 |
| STATE | PA4 | 状态检测 |
| EN | PA5 | AT模式使能 |

### PWM输出
| 通道 | GPIO | 说明 |
|------|------|------|
| TIM3_CH1 | PA6 | PWM输出1 |
| TIM4_CH1 | PB6 | PWM输出2 |

## 目录结构

```
TFT_EC11_KEY/
├── app/                    # 应用层
│   ├── main.c              # 简单示例
│   ├── main_app.c          # 完整应用示例
│   └── main_example.c      # 菜单系统示例
│
├── board/                  # 板级支持
│   ├── board.c/h           # 系统初始化、延时
│
├── bsp/                    # BSP驱动层 (标准库)
│   ├── bsp_adc.c/h         # ADC驱动
│   ├── bsp_ec11.c/h        # EC11编码器
│   ├── bsp_key.c/h         # 按键驱动
│   ├── bsp_tft_st7789.c/h  # TFT显示驱动
│   ├── bsp_uart.c/h        # 串口驱动
│   ├── bsp_bluetooth.c/h   # 蓝牙驱动
│   ├── bsp_pwm.c/h         # PWM驱动
│   ├── bsp_timer.c/h       # 定时器驱动
│   └── bsp_u8g2_port.c/h   # U8G2适配层
│
├── bsp_hal/                # BSP驱动层 (HAL库)
│   ├── bsp_adc_hal.h       # ADC (HAL版)
│   ├── bsp_ec11_hal.h      # EC11 (HAL版)
│   ├── bsp_tft_hal.h       # TFT (HAL版)
│   └── bsp_uart_hal.h      # UART (HAL版)
│
├── middleware/             # 中间件层
│   ├── scheduler.c/h       # 伪调度器
│   ├── menu_core.c/h       # 菜单核心
│   ├── menu_config.c/h     # EEPROM配置
│   ├── menu_animation.c/h  # 动画效果
│   ├── menu_dynamic.c/h    # 动态菜单
│   └── waveform_display.c/h # 波形显示
│
├── doc/                    # 文档
│   ├── API参考手册.md
│   ├── 项目说明.md
│   ├── 高级功能说明.md
│   └── 移植使用手册.md
│
├── libraries/              # 标准库和CMSIS
├── module/                 # 中断和配置
├── project/                # Keil工程
└── README.md
```

## 快速开始

### 1. 使用调度器 (推荐)

```c
#include "middleware/scheduler.h"

// 定义任务
void task_ec11(void *arg) {
    ec11_event_t event = bsp_ec11_scan();
    // 处理事件...
}

void task_display(void *arg) {
    // 更新显示...
}

int main(void) {
    board_init();
    scheduler_init();

    // 创建任务
    task_config_t cfg;
    cfg = TASK_PERIODIC("EC11", task_ec11, 10, TASK_PRIORITY_HIGH);
    scheduler_task_create(&cfg);

    cfg = TASK_PERIODIC("Display", task_display, 50, TASK_PRIORITY_NORMAL);
    scheduler_task_create(&cfg);

    // 启动调度器
    scheduler_start();
}
```

### 2. 使用ADC和波形显示

```c
#include "bsp/bsp_adc.h"
#include "middleware/waveform_display.h"

// 初始化ADC
adc_channel_config_t cfg = bsp_adc_get_default_config();
bsp_adc_init(&cfg);

// 读取电压
uint32_t voltage = bsp_adc_read_voltage();  // mV

// 初始化波形显示
waveform_init(&adc_source, &display_interface);
waveform_start();

// 在任务中更新
waveform_update();
```

### 3. 使用蓝牙

```c
#include "bsp/bsp_bluetooth.h"

bsp_bluetooth_init();
bsp_bluetooth_set_frame_callback(on_frame_received);

// 发送数据帧
uint8_t data[] = {0x01, 0x02};
bsp_bluetooth_send_frame(BT_CMD_ADC_DATA, data, 2);

// 在任务中处理
bsp_bluetooth_process();
```

## API文档

详见 `doc/API参考手册.md`

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 2.0.0 | 2025-12-12 | 添加调度器、TFT、蓝牙、ADC波形等功能 |
| 1.0.0 | 2024-03-07 | 初始版本，EC11+菜单系统 |

## 移植

1. 修改 `bsp/` 中的引脚定义
2. 实现 `delay_ms()` 和 `delay_us()` 函数
3. 修改时钟配置
4. 中间件层无需修改

详见 `doc/移植使用手册.md`

## License

MIT License

---

**Author**: Claude Code
**Platform**: STM32F407VGT6
