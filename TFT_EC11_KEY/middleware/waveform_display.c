/**
 * @file waveform_display.c
 * @brief 波形显示模块实现 - 简易示波器功能
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "waveform_display.h"
#include <string.h>
#include <stdlib.h>

/*=============================================================================
 *                              私有宏定义
 *============================================================================*/

#define ABS(x)      ((x) < 0 ? -(x) : (x))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/*=============================================================================
 *                              时基表
 *============================================================================*/

/* 时基对应的采样率 (Hz) */
static const uint32_t timebase_sample_rate[] = {
    100000,     /* TIMEBASE_100US: 100kHz */
    50000,      /* TIMEBASE_200US: 50kHz */
    20000,      /* TIMEBASE_500US: 20kHz */
    10000,      /* TIMEBASE_1MS: 10kHz */
    5000,       /* TIMEBASE_2MS: 5kHz */
    2000,       /* TIMEBASE_5MS: 2kHz */
    1000,       /* TIMEBASE_10MS: 1kHz */
    500,        /* TIMEBASE_20MS: 500Hz */
    200,        /* TIMEBASE_50MS: 200Hz */
    100,        /* TIMEBASE_100MS: 100Hz */
    50,         /* TIMEBASE_200MS: 50Hz */
    20,         /* TIMEBASE_500MS: 20Hz */
    10          /* TIMEBASE_1S: 10Hz */
};

/* 时基字符串 */
static const char* timebase_str[] = {
    "100us", "200us", "500us",
    "1ms", "2ms", "5ms", "10ms", "20ms", "50ms",
    "100ms", "200ms", "500ms", "1s"
};

/* 电压档位 (mV/div) */
static const uint16_t voltage_div_mv[] = {
    100, 200, 500, 1000, 2000, 0 /* AUTO */
};

/* 电压档位字符串 */
static const char* voltage_div_str[] = {
    "100mV", "200mV", "500mV", "1V", "2V", "AUTO"
};

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static waveform_config_t config = {
    .trigger_mode = TRIGGER_MODE_AUTO,
    .trigger_edge = TRIGGER_EDGE_RISING,
    .trigger_level = 1650,  /* 中间电平 */
    .timebase = TIMEBASE_1MS,
    .voltage_div = VOLTAGE_DIV_AUTO,
    .display_mode = DISPLAY_MODE_LINES,
    .show_grid = 1,
    .show_measurement = 1,
    .x_offset = 0,
    .y_offset = 0
};

static waveform_state_t state = {0};

static uint16_t waveform_buffer[WAVEFORM_BUFFER_SIZE];
static uint16_t stored_waveforms[WAVEFORM_MAX_STORED][WAVEFORM_BUFFER_SIZE];
static uint8_t stored_valid[WAVEFORM_MAX_STORED] = {0};

static const waveform_data_source_t *data_src = NULL;
static const waveform_display_interface_t *disp = NULL;

static uint8_t need_refresh = 0;
static uint16_t auto_voltage_div_mv = 1000;

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static void calculate_measurement(void);
static uint32_t calculate_frequency(void);
static int16_t voltage_to_y(uint16_t voltage_mv);
static void draw_grid(void);
static void draw_waveform(void);
static void draw_status_bar(void);
static void draw_measurement(void);
static int find_trigger_point(void);
static void auto_voltage_scale(void);

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief 初始化波形显示模块
 */
int waveform_init(const waveform_data_source_t *data_source,
                  const waveform_display_interface_t *display)
{
    if (data_source == NULL || display == NULL) {
        return -1;
    }

    data_src = data_source;
    disp = display;

    /* 初始化数据源 */
    if (data_src->init != NULL) {
        if (data_src->init() != 0) {
            return -1;
        }
    }

    /* 清空缓冲区 */
    memset(waveform_buffer, 0, sizeof(waveform_buffer));
    memset(&state, 0, sizeof(state));

    state.sample_count = WAVEFORM_BUFFER_SIZE;
    state.sample_rate = timebase_sample_rate[config.timebase];

    return 0;
}

/**
 * @brief 反初始化
 */
void waveform_deinit(void)
{
    waveform_stop();
    if (data_src != NULL && data_src->deinit != NULL) {
        data_src->deinit();
    }
    data_src = NULL;
    disp = NULL;
}

/**
 * @brief 开始采样/显示
 */
void waveform_start(void)
{
    if (data_src == NULL) return;

    /* 设置采样率 */
    if (data_src->set_sample_rate != NULL) {
        data_src->set_sample_rate(timebase_sample_rate[config.timebase]);
    }

    state.is_running = 1;
    state.is_triggered = 0;
    need_refresh = 1;
}

/**
 * @brief 停止采样/显示
 */
void waveform_stop(void)
{
    state.is_running = 0;
}

/**
 * @brief 单次采样
 */
void waveform_single(void)
{
    config.trigger_mode = TRIGGER_MODE_SINGLE;
    state.is_triggered = 0;
    state.is_running = 1;
}

/**
 * @brief 波形更新
 */
void waveform_update(void)
{
    uint32_t i;
    int trigger_point;

    if (data_src == NULL || disp == NULL) return;
    if (!state.is_running && !need_refresh) return;

    if (state.is_running) {
        /* 采集数据 */
        if (data_src->read_buffer != NULL) {
            data_src->read_buffer(waveform_buffer, WAVEFORM_BUFFER_SIZE);
        } else if (data_src->read != NULL) {
            for (i = 0; i < WAVEFORM_BUFFER_SIZE; i++) {
                waveform_buffer[i] = data_src->read();
            }
        }

        /* 触发处理 */
        if (config.trigger_mode != TRIGGER_MODE_NONE) {
            trigger_point = find_trigger_point();
            if (trigger_point >= 0) {
                state.is_triggered = 1;
                /* 重新排列数据，使触发点在开头 */
                if (trigger_point > 0 && trigger_point < WAVEFORM_BUFFER_SIZE / 2) {
                    memmove(waveform_buffer, &waveform_buffer[trigger_point],
                            (WAVEFORM_BUFFER_SIZE - trigger_point) * sizeof(uint16_t));
                }
            } else if (config.trigger_mode == TRIGGER_MODE_NORMAL) {
                /* 普通模式未触发时不更新显示 */
                return;
            }
        }

        /* 自动量程 */
        if (config.voltage_div == VOLTAGE_DIV_AUTO) {
            auto_voltage_scale();
        }

        /* 计算测量值 */
        calculate_measurement();

        /* 单次触发后停止 */
        if (config.trigger_mode == TRIGGER_MODE_SINGLE && state.is_triggered) {
            state.is_running = 0;
        }
    }

    /* 绘制波形 */
    if (disp->clear != NULL) {
        disp->clear();
    }

    /* 绘制网格 */
    if (config.show_grid) {
        draw_grid();
    }

    /* 绘制波形 */
    draw_waveform();

    /* 绘制状态栏 */
    draw_status_bar();

    /* 绘制测量值 */
    if (config.show_measurement) {
        draw_measurement();
    }

    /* 更新显示 */
    if (disp->update != NULL) {
        disp->update();
    }

    need_refresh = 0;
}

/**
 * @brief 设置配置
 */
void waveform_set_config(const waveform_config_t *cfg)
{
    if (cfg != NULL) {
        config = *cfg;
        need_refresh = 1;
    }
}

/**
 * @brief 获取当前配置
 */
const waveform_config_t* waveform_get_config(void)
{
    return &config;
}

/**
 * @brief 获取状态
 */
const waveform_state_t* waveform_get_state(void)
{
    return &state;
}

/**
 * @brief 获取测量值
 */
const waveform_measurement_t* waveform_get_measurement(void)
{
    return &state.measurement;
}

/**
 * @brief 设置时基
 */
void waveform_set_timebase(waveform_timebase_t timebase)
{
    if (timebase < TIMEBASE_COUNT) {
        config.timebase = timebase;
        state.sample_rate = timebase_sample_rate[timebase];
        if (data_src != NULL && data_src->set_sample_rate != NULL) {
            data_src->set_sample_rate(state.sample_rate);
        }
        need_refresh = 1;
    }
}

/**
 * @brief 设置电压档位
 */
void waveform_set_voltage_div(waveform_voltage_div_t div)
{
    if (div < VOLTAGE_DIV_COUNT) {
        config.voltage_div = div;
        need_refresh = 1;
    }
}

/**
 * @brief 设置触发模式
 */
void waveform_set_trigger_mode(waveform_trigger_mode_t mode)
{
    config.trigger_mode = mode;
    state.is_triggered = 0;
    need_refresh = 1;
}

/**
 * @brief 设置触发电平
 */
void waveform_set_trigger_level(uint16_t level_mv)
{
    config.trigger_level = level_mv;
    need_refresh = 1;
}

/**
 * @brief 设置触发边沿
 */
void waveform_set_trigger_edge(waveform_trigger_edge_t edge)
{
    config.trigger_edge = edge;
    need_refresh = 1;
}

/**
 * @brief 设置显示模式
 */
void waveform_set_display_mode(waveform_display_mode_t mode)
{
    config.display_mode = mode;
    need_refresh = 1;
}

/**
 * @brief 调整X偏移
 */
void waveform_set_x_offset(int8_t offset)
{
    config.x_offset = offset;
    need_refresh = 1;
}

/**
 * @brief 调整Y偏移
 */
void waveform_set_y_offset(int8_t offset)
{
    config.y_offset = offset;
    need_refresh = 1;
}

/**
 * @brief 自动设置
 */
void waveform_auto_setup(void)
{
    config.voltage_div = VOLTAGE_DIV_AUTO;
    config.trigger_mode = TRIGGER_MODE_AUTO;
    config.trigger_edge = TRIGGER_EDGE_RISING;
    config.x_offset = 0;
    config.y_offset = 0;
    need_refresh = 1;
}

/**
 * @brief 存储当前波形
 */
int waveform_store(uint8_t slot)
{
    if (slot >= WAVEFORM_MAX_STORED) {
        return -1;
    }
    memcpy(stored_waveforms[slot], waveform_buffer, sizeof(waveform_buffer));
    stored_valid[slot] = 1;
    return 0;
}

/**
 * @brief 回放存储的波形
 */
int waveform_recall(uint8_t slot)
{
    if (slot >= WAVEFORM_MAX_STORED || !stored_valid[slot]) {
        return -1;
    }
    memcpy(waveform_buffer, stored_waveforms[slot], sizeof(waveform_buffer));
    waveform_stop();
    need_refresh = 1;
    return 0;
}

/**
 * @brief 获取波形数据缓冲区
 */
const uint16_t* waveform_get_buffer(void)
{
    return waveform_buffer;
}

/**
 * @brief 获取波形数据长度
 */
uint16_t waveform_get_buffer_length(void)
{
    return WAVEFORM_BUFFER_SIZE;
}

/**
 * @brief 强制刷新显示
 */
void waveform_force_refresh(void)
{
    need_refresh = 1;
}

/**
 * @brief 切换显示网格
 */
void waveform_toggle_grid(void)
{
    config.show_grid = !config.show_grid;
    need_refresh = 1;
}

/**
 * @brief 切换显示测量值
 */
void waveform_toggle_measurement(void)
{
    config.show_measurement = !config.show_measurement;
    need_refresh = 1;
}

/**
 * @brief 时基增加
 */
void waveform_timebase_increase(void)
{
    if (config.timebase < TIMEBASE_1S) {
        waveform_set_timebase((waveform_timebase_t)(config.timebase + 1));
    }
}

/**
 * @brief 时基减少
 */
void waveform_timebase_decrease(void)
{
    if (config.timebase > TIMEBASE_100US) {
        waveform_set_timebase((waveform_timebase_t)(config.timebase - 1));
    }
}

/**
 * @brief 电压档位增加
 */
void waveform_voltage_div_increase(void)
{
    if (config.voltage_div < VOLTAGE_DIV_AUTO) {
        waveform_set_voltage_div((waveform_voltage_div_t)(config.voltage_div + 1));
    }
}

/**
 * @brief 电压档位减少
 */
void waveform_voltage_div_decrease(void)
{
    if (config.voltage_div > VOLTAGE_DIV_100MV) {
        waveform_set_voltage_div((waveform_voltage_div_t)(config.voltage_div - 1));
    }
}

/**
 * @brief 获取时基字符串描述
 */
const char* waveform_get_timebase_str(waveform_timebase_t timebase)
{
    if (timebase < TIMEBASE_COUNT) {
        return timebase_str[timebase];
    }
    return "???";
}

/**
 * @brief 获取电压档位字符串描述
 */
const char* waveform_get_voltage_div_str(waveform_voltage_div_t div)
{
    if (div < VOLTAGE_DIV_COUNT) {
        return voltage_div_str[div];
    }
    return "???";
}

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

/**
 * @brief 计算测量值
 */
static void calculate_measurement(void)
{
    uint32_t i;
    uint32_t sum = 0;
    uint64_t sum_sq = 0;
    uint16_t vmax = 0, vmin = 4095;
    uint16_t val;
    uint32_t voltage_mv;

    /* 计算最大、最小、平均值 */
    for (i = 0; i < WAVEFORM_BUFFER_SIZE; i++) {
        val = waveform_buffer[i];
        if (val > vmax) vmax = val;
        if (val < vmin) vmin = val;
        sum += val;
        sum_sq += (uint32_t)val * val;
    }

    /* 转换为mV */
    state.measurement.vmax = (uint16_t)((uint32_t)vmax * 3300 / 4096);
    state.measurement.vmin = (uint16_t)((uint32_t)vmin * 3300 / 4096);
    state.measurement.vpp = state.measurement.vmax - state.measurement.vmin;
    state.measurement.vavg = (uint16_t)((sum * 3300 / 4096) / WAVEFORM_BUFFER_SIZE);

    /* 计算RMS */
    voltage_mv = (uint32_t)(sum_sq / WAVEFORM_BUFFER_SIZE);
    voltage_mv = voltage_mv * 3300 * 3300 / 4096 / 4096;
    /* 简化的平方根计算 */
    {
        uint32_t root = voltage_mv;
        uint32_t x = voltage_mv;
        if (x > 0) {
            for (i = 0; i < 10; i++) {
                root = (root + x / root) / 2;
            }
        }
        state.measurement.vrms = (uint16_t)root;
    }

    /* 计算频率 */
    state.measurement.frequency = calculate_frequency();
    if (state.measurement.frequency > 0) {
        state.measurement.period = 1000000 / state.measurement.frequency;
    } else {
        state.measurement.period = 0;
    }
}

/**
 * @brief 计算频率 (过零检测法)
 */
static uint32_t calculate_frequency(void)
{
    uint32_t i;
    uint16_t mid = (state.measurement.vmax + state.measurement.vmin) / 2;
    uint16_t threshold = state.measurement.vpp / 10; /* 10%滞回 */
    uint32_t rising_count = 0;
    uint32_t first_rising = 0, last_rising = 0;
    int last_state = 0;
    uint16_t val_mv;

    /* 峰峰值太小，无法测量频率 */
    if (state.measurement.vpp < 100) {
        return 0;
    }

    /* 转换回ADC值进行比较 */
    mid = mid * 4096 / 3300;
    threshold = threshold * 4096 / 3300;
    if (threshold < 10) threshold = 10;

    for (i = 0; i < WAVEFORM_BUFFER_SIZE; i++) {
        val_mv = waveform_buffer[i];

        if (last_state <= 0 && val_mv > mid + threshold) {
            /* 上升沿 */
            if (rising_count == 0) {
                first_rising = i;
            }
            last_rising = i;
            rising_count++;
            last_state = 1;
        } else if (last_state >= 0 && val_mv < mid - threshold) {
            last_state = -1;
        }
    }

    if (rising_count < 2) {
        return 0;
    }

    /* 计算频率 */
    uint32_t samples_per_cycle = (last_rising - first_rising) / (rising_count - 1);
    if (samples_per_cycle == 0) {
        return 0;
    }

    return state.sample_rate / samples_per_cycle;
}

/**
 * @brief 电压值转换为Y坐标
 */
static int16_t voltage_to_y(uint16_t voltage_mv)
{
    int32_t y;
    uint16_t div_mv;

    /* 获取当前电压档位 */
    if (config.voltage_div == VOLTAGE_DIV_AUTO) {
        div_mv = auto_voltage_div_mv;
    } else {
        div_mv = voltage_div_mv[config.voltage_div];
    }

    /* 计算Y坐标 (屏幕中心为参考点) */
    /* 电压范围: 0 ~ div_mv * GRID_Y_DIV */
    y = WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT / 2;
    y -= (int32_t)(voltage_mv - 1650) * WAVEFORM_PLOT_HEIGHT / (div_mv * WAVEFORM_GRID_Y_DIV);
    y += config.y_offset;

    /* 限制范围 */
    if (y < WAVEFORM_PLOT_Y) y = WAVEFORM_PLOT_Y;
    if (y > WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT - 1) {
        y = WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT - 1;
    }

    return (int16_t)y;
}

/**
 * @brief 绘制网格
 */
static void draw_grid(void)
{
    int16_t x, y;
    int16_t grid_x_step = WAVEFORM_PLOT_WIDTH / WAVEFORM_GRID_X_DIV;
    int16_t grid_y_step = WAVEFORM_PLOT_HEIGHT / WAVEFORM_GRID_Y_DIV;

    if (disp->set_color != NULL) {
        disp->set_color(1);
    }

    /* 绘制垂直网格线 */
    for (x = WAVEFORM_PLOT_X; x <= WAVEFORM_PLOT_X + WAVEFORM_PLOT_WIDTH; x += grid_x_step) {
        /* 点状虚线 */
        for (y = WAVEFORM_PLOT_Y; y < WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT; y += 4) {
            if (disp->draw_pixel != NULL) {
                disp->draw_pixel(x, y);
            }
        }
    }

    /* 绘制水平网格线 */
    for (y = WAVEFORM_PLOT_Y; y <= WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT; y += grid_y_step) {
        /* 点状虚线 */
        for (x = WAVEFORM_PLOT_X; x < WAVEFORM_PLOT_X + WAVEFORM_PLOT_WIDTH; x += 4) {
            if (disp->draw_pixel != NULL) {
                disp->draw_pixel(x, y);
            }
        }
    }

    /* 绘制中心线 (实线) */
    if (disp->draw_hline != NULL) {
        disp->draw_hline(WAVEFORM_PLOT_X, WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT / 2, WAVEFORM_PLOT_WIDTH);
    }
}

/**
 * @brief 绘制波形
 */
static void draw_waveform(void)
{
    uint32_t i;
    int16_t x0, y0, x1, y1;
    uint16_t voltage_mv;
    uint32_t sample_step;

    if (disp->set_color != NULL) {
        disp->set_color(1);
    }

    /* 计算采样步进 (将WAVEFORM_BUFFER_SIZE个点映射到WAVEFORM_PLOT_WIDTH) */
    sample_step = (WAVEFORM_BUFFER_SIZE * 256) / WAVEFORM_PLOT_WIDTH;

    x0 = WAVEFORM_PLOT_X + config.x_offset;
    voltage_mv = (uint32_t)waveform_buffer[0] * 3300 / 4096;
    y0 = voltage_to_y(voltage_mv);

    for (i = 1; i < WAVEFORM_PLOT_WIDTH; i++) {
        uint32_t sample_idx = (i * sample_step) / 256;
        if (sample_idx >= WAVEFORM_BUFFER_SIZE) {
            sample_idx = WAVEFORM_BUFFER_SIZE - 1;
        }

        x1 = WAVEFORM_PLOT_X + i + config.x_offset;
        voltage_mv = (uint32_t)waveform_buffer[sample_idx] * 3300 / 4096;
        y1 = voltage_to_y(voltage_mv);

        /* 限制X范围 */
        if (x1 < WAVEFORM_PLOT_X || x1 >= WAVEFORM_PLOT_X + WAVEFORM_PLOT_WIDTH) {
            x0 = x1;
            y0 = y1;
            continue;
        }

        switch (config.display_mode) {
        case DISPLAY_MODE_DOTS:
            if (disp->draw_pixel != NULL) {
                disp->draw_pixel(x1, y1);
            }
            break;

        case DISPLAY_MODE_LINES:
            if (disp->draw_line != NULL && x0 >= WAVEFORM_PLOT_X) {
                disp->draw_line(x0, y0, x1, y1);
            }
            break;

        case DISPLAY_MODE_FILLED:
            if (disp->draw_vline != NULL) {
                int16_t y_mid = WAVEFORM_PLOT_Y + WAVEFORM_PLOT_HEIGHT / 2;
                int16_t y_start = MIN(y1, y_mid);
                int16_t y_end = MAX(y1, y_mid);
                disp->draw_vline(x1, y_start, y_end - y_start);
            }
            break;
        }

        x0 = x1;
        y0 = y1;
    }

    /* 绘制触发电平线 */
    if (config.trigger_mode != TRIGGER_MODE_NONE) {
        int16_t trig_y = voltage_to_y(config.trigger_level);
        if (disp->draw_hline != NULL) {
            /* 用虚线表示触发电平 */
            for (x0 = WAVEFORM_PLOT_X; x0 < WAVEFORM_PLOT_X + WAVEFORM_PLOT_WIDTH; x0 += 8) {
                if (disp->draw_pixel != NULL) {
                    disp->draw_pixel(x0, trig_y);
                    disp->draw_pixel(x0 + 1, trig_y);
                }
            }
        }
    }
}

/**
 * @brief 绘制状态栏
 */
static void draw_status_bar(void)
{
    char str[32];
    const char *tb_str, *vd_str;

    if (disp->draw_string == NULL) return;

    /* 时基 */
    tb_str = waveform_get_timebase_str(config.timebase);

    /* 电压档位 */
    if (config.voltage_div == VOLTAGE_DIV_AUTO) {
        /* 显示自动计算的档位 */
        if (auto_voltage_div_mv >= 1000) {
            snprintf(str, sizeof(str), "%dV", (int)(auto_voltage_div_mv / 1000));
        } else {
            snprintf(str, sizeof(str), "%dmV", (int)auto_voltage_div_mv);
        }
        vd_str = str;
    } else {
        vd_str = waveform_get_voltage_div_str(config.voltage_div);
    }

    /* 显示状态 */
    disp->draw_string(0, 0, tb_str);
    disp->draw_string(40, 0, vd_str);

    /* 触发状态 */
    if (state.is_triggered) {
        disp->draw_string(80, 0, "T");
    }

    /* 运行状态 */
    if (state.is_running) {
        disp->draw_string(90, 0, "RUN");
    } else {
        disp->draw_string(90, 0, "STOP");
    }
}

/**
 * @brief 绘制测量值
 */
static void draw_measurement(void)
{
    char str[32];

    if (disp->draw_string == NULL) return;

    /* 在底部显示测量值 */
    snprintf(str, sizeof(str), "Vpp:%dmV", (int)state.measurement.vpp);
    disp->draw_string(0, 58, str);

    if (state.measurement.frequency > 0) {
        if (state.measurement.frequency >= 1000) {
            snprintf(str, sizeof(str), "F:%dkHz", (int)(state.measurement.frequency / 1000));
        } else {
            snprintf(str, sizeof(str), "F:%dHz", (int)state.measurement.frequency);
        }
        disp->draw_string(70, 58, str);
    }
}

/**
 * @brief 查找触发点
 */
static int find_trigger_point(void)
{
    uint32_t i;
    uint16_t trig_adc = (uint32_t)config.trigger_level * 4096 / 3300;
    uint16_t hysteresis = 50; /* 滞回 */

    for (i = 1; i < WAVEFORM_BUFFER_SIZE / 2; i++) {
        switch (config.trigger_edge) {
        case TRIGGER_EDGE_RISING:
            if (waveform_buffer[i-1] < trig_adc - hysteresis &&
                waveform_buffer[i] >= trig_adc) {
                return (int)i;
            }
            break;

        case TRIGGER_EDGE_FALLING:
            if (waveform_buffer[i-1] > trig_adc + hysteresis &&
                waveform_buffer[i] <= trig_adc) {
                return (int)i;
            }
            break;

        case TRIGGER_EDGE_BOTH:
            if ((waveform_buffer[i-1] < trig_adc - hysteresis &&
                 waveform_buffer[i] >= trig_adc) ||
                (waveform_buffer[i-1] > trig_adc + hysteresis &&
                 waveform_buffer[i] <= trig_adc)) {
                return (int)i;
            }
            break;
        }
    }

    return -1; /* 未找到触发点 */
}

/**
 * @brief 自动调整电压量程
 */
static void auto_voltage_scale(void)
{
    uint16_t vpp = state.measurement.vpp;

    /* 根据峰峰值选择合适的量程 */
    if (vpp < 300) {
        auto_voltage_div_mv = 100;
    } else if (vpp < 600) {
        auto_voltage_div_mv = 200;
    } else if (vpp < 1500) {
        auto_voltage_div_mv = 500;
    } else if (vpp < 3000) {
        auto_voltage_div_mv = 1000;
    } else {
        auto_voltage_div_mv = 2000;
    }
}
