/**
 * @file waveform_display.h
 * @brief 波形显示模块 - 简易示波器功能
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 功能特性:
 *       - 实时波形显示
 *       - 自动量程调整
 *       - 触发功能
 *       - 时基调整
 *       - 电压/频率测量
 *       - 波形存储与回放
 *       - 支持U8G2和TFT显示
 */

#ifndef __WAVEFORM_DISPLAY_H
#define __WAVEFORM_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/* 波形缓冲区大小 */
#define WAVEFORM_BUFFER_SIZE        256

/* 显示区域配置 */
#define WAVEFORM_DISPLAY_X          0
#define WAVEFORM_DISPLAY_Y          0
#define WAVEFORM_DISPLAY_WIDTH      128
#define WAVEFORM_DISPLAY_HEIGHT     64

/* 波形显示区域 (排除状态栏) */
#define WAVEFORM_PLOT_X             0
#define WAVEFORM_PLOT_Y             10
#define WAVEFORM_PLOT_WIDTH         128
#define WAVEFORM_PLOT_HEIGHT        48

/* 网格配置 */
#define WAVEFORM_GRID_X_DIV         8       /**< X轴分格数 */
#define WAVEFORM_GRID_Y_DIV         4       /**< Y轴分格数 */

/* 最大存储波形数 */
#define WAVEFORM_MAX_STORED         4

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief 触发模式
 */
typedef enum {
    TRIGGER_MODE_AUTO,          /**< 自动触发 */
    TRIGGER_MODE_NORMAL,        /**< 普通触发 */
    TRIGGER_MODE_SINGLE,        /**< 单次触发 */
    TRIGGER_MODE_NONE           /**< 无触发(自由运行) */
} waveform_trigger_mode_t;

/**
 * @brief 触发边沿
 */
typedef enum {
    TRIGGER_EDGE_RISING,        /**< 上升沿 */
    TRIGGER_EDGE_FALLING,       /**< 下降沿 */
    TRIGGER_EDGE_BOTH           /**< 双边沿 */
} waveform_trigger_edge_t;

/**
 * @brief 时基设置 (每格时间)
 */
typedef enum {
    TIMEBASE_100US,             /**< 100us/div */
    TIMEBASE_200US,             /**< 200us/div */
    TIMEBASE_500US,             /**< 500us/div */
    TIMEBASE_1MS,               /**< 1ms/div */
    TIMEBASE_2MS,               /**< 2ms/div */
    TIMEBASE_5MS,               /**< 5ms/div */
    TIMEBASE_10MS,              /**< 10ms/div */
    TIMEBASE_20MS,              /**< 20ms/div */
    TIMEBASE_50MS,              /**< 50ms/div */
    TIMEBASE_100MS,             /**< 100ms/div */
    TIMEBASE_200MS,             /**< 200ms/div */
    TIMEBASE_500MS,             /**< 500ms/div */
    TIMEBASE_1S,                /**< 1s/div */
    TIMEBASE_COUNT
} waveform_timebase_t;

/**
 * @brief 电压档位 (每格电压)
 */
typedef enum {
    VOLTAGE_DIV_100MV,          /**< 100mV/div */
    VOLTAGE_DIV_200MV,          /**< 200mV/div */
    VOLTAGE_DIV_500MV,          /**< 500mV/div */
    VOLTAGE_DIV_1V,             /**< 1V/div */
    VOLTAGE_DIV_2V,             /**< 2V/div */
    VOLTAGE_DIV_AUTO,           /**< 自动量程 */
    VOLTAGE_DIV_COUNT
} waveform_voltage_div_t;

/**
 * @brief 显示模式
 */
typedef enum {
    DISPLAY_MODE_DOTS,          /**< 点显示 */
    DISPLAY_MODE_LINES,         /**< 线条连接 */
    DISPLAY_MODE_FILLED         /**< 填充模式 */
} waveform_display_mode_t;

/**
 * @brief 测量结果
 */
typedef struct {
    uint16_t vmax;              /**< 最大电压 (mV) */
    uint16_t vmin;              /**< 最小电压 (mV) */
    uint16_t vpp;               /**< 峰峰值 (mV) */
    uint16_t vavg;              /**< 平均值 (mV) */
    uint16_t vrms;              /**< RMS值 (mV) */
    uint32_t frequency;         /**< 频率 (Hz) */
    uint32_t period;            /**< 周期 (us) */
    uint8_t duty_cycle;         /**< 占空比 (%) */
} waveform_measurement_t;

/**
 * @brief 波形配置
 */
typedef struct {
    waveform_trigger_mode_t trigger_mode;
    waveform_trigger_edge_t trigger_edge;
    uint16_t trigger_level;     /**< 触发电平 (mV) */
    waveform_timebase_t timebase;
    waveform_voltage_div_t voltage_div;
    waveform_display_mode_t display_mode;
    uint8_t show_grid;          /**< 显示网格 */
    uint8_t show_measurement;   /**< 显示测量值 */
    int8_t x_offset;            /**< X轴偏移 */
    int8_t y_offset;            /**< Y轴偏移 */
} waveform_config_t;

/**
 * @brief 波形状态
 */
typedef struct {
    uint8_t is_running;         /**< 运行状态 */
    uint8_t is_triggered;       /**< 触发状态 */
    uint16_t sample_count;      /**< 采样点数 */
    uint32_t sample_rate;       /**< 实际采样率 */
    waveform_measurement_t measurement;
} waveform_state_t;

/**
 * @brief 数据源接口
 */
typedef struct {
    int (*init)(void);
    void (*deinit)(void);
    uint16_t (*read)(void);
    int (*read_buffer)(uint16_t *buf, uint32_t len);
    void (*set_sample_rate)(uint32_t rate);
} waveform_data_source_t;

/**
 * @brief 显示接口
 */
typedef struct {
    void (*clear)(void);
    void (*draw_pixel)(int16_t x, int16_t y);
    void (*draw_line)(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void (*draw_hline)(int16_t x, int16_t y, int16_t w);
    void (*draw_vline)(int16_t x, int16_t y, int16_t h);
    void (*draw_rect)(int16_t x, int16_t y, int16_t w, int16_t h);
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h);
    void (*draw_string)(int16_t x, int16_t y, const char *str);
    void (*update)(void);
    void (*set_color)(uint8_t color);
} waveform_display_interface_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief 初始化波形显示模块
 * @param data_source 数据源接口
 * @param display 显示接口
 * @retval 0:成功 -1:失败
 */
int waveform_init(const waveform_data_source_t *data_source,
                  const waveform_display_interface_t *display);

/**
 * @brief 反初始化
 */
void waveform_deinit(void);

/**
 * @brief 开始采样/显示
 */
void waveform_start(void);

/**
 * @brief 停止采样/显示
 */
void waveform_stop(void);

/**
 * @brief 单次采样
 */
void waveform_single(void);

/**
 * @brief 波形更新 (主循环调用)
 * @note 建议调用周期: 20-50ms
 */
void waveform_update(void);

/**
 * @brief 设置配置
 * @param config 配置结构体
 */
void waveform_set_config(const waveform_config_t *config);

/**
 * @brief 获取当前配置
 * @retval 配置结构体指针
 */
const waveform_config_t* waveform_get_config(void);

/**
 * @brief 获取状态
 * @retval 状态结构体指针
 */
const waveform_state_t* waveform_get_state(void);

/**
 * @brief 获取测量值
 * @retval 测量结果指针
 */
const waveform_measurement_t* waveform_get_measurement(void);

/**
 * @brief 设置时基
 * @param timebase 时基设置
 */
void waveform_set_timebase(waveform_timebase_t timebase);

/**
 * @brief 设置电压档位
 * @param div 电压档位
 */
void waveform_set_voltage_div(waveform_voltage_div_t div);

/**
 * @brief 设置触发模式
 * @param mode 触发模式
 */
void waveform_set_trigger_mode(waveform_trigger_mode_t mode);

/**
 * @brief 设置触发电平
 * @param level_mv 触发电平 (mV)
 */
void waveform_set_trigger_level(uint16_t level_mv);

/**
 * @brief 设置触发边沿
 * @param edge 触发边沿
 */
void waveform_set_trigger_edge(waveform_trigger_edge_t edge);

/**
 * @brief 设置显示模式
 * @param mode 显示模式
 */
void waveform_set_display_mode(waveform_display_mode_t mode);

/**
 * @brief 调整X偏移
 * @param offset 偏移量 (-128 ~ 127)
 */
void waveform_set_x_offset(int8_t offset);

/**
 * @brief 调整Y偏移
 * @param offset 偏移量 (-128 ~ 127)
 */
void waveform_set_y_offset(int8_t offset);

/**
 * @brief 自动设置 (自动调整量程和触发)
 */
void waveform_auto_setup(void);

/**
 * @brief 存储当前波形
 * @param slot 存储槽位 (0 ~ WAVEFORM_MAX_STORED-1)
 * @retval 0:成功 -1:失败
 */
int waveform_store(uint8_t slot);

/**
 * @brief 回放存储的波形
 * @param slot 存储槽位
 * @retval 0:成功 -1:失败
 */
int waveform_recall(uint8_t slot);

/**
 * @brief 获取波形数据缓冲区
 * @retval 波形数据缓冲区指针
 */
const uint16_t* waveform_get_buffer(void);

/**
 * @brief 获取波形数据长度
 * @retval 数据长度
 */
uint16_t waveform_get_buffer_length(void);

/**
 * @brief 强制刷新显示
 */
void waveform_force_refresh(void);

/**
 * @brief 切换显示网格
 */
void waveform_toggle_grid(void);

/**
 * @brief 切换显示测量值
 */
void waveform_toggle_measurement(void);

/**
 * @brief 时基增加
 */
void waveform_timebase_increase(void);

/**
 * @brief 时基减少
 */
void waveform_timebase_decrease(void);

/**
 * @brief 电压档位增加
 */
void waveform_voltage_div_increase(void);

/**
 * @brief 电压档位减少
 */
void waveform_voltage_div_decrease(void);

/**
 * @brief 获取时基字符串描述
 * @param timebase 时基值
 * @retval 字符串指针
 */
const char* waveform_get_timebase_str(waveform_timebase_t timebase);

/**
 * @brief 获取电压档位字符串描述
 * @param div 电压档位
 * @retval 字符串指针
 */
const char* waveform_get_voltage_div_str(waveform_voltage_div_t div);

#ifdef __cplusplus
}
#endif

#endif /* __WAVEFORM_DISPLAY_H */
