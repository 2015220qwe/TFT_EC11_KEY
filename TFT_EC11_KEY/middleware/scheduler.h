/**
 * @file scheduler.h
 * @brief 伪调度器模块 - 协作式任务调度系统
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 功能特性:
 *       - 协作式多任务调度
 *       - 支持周期性任务和一次性任务
 *       - 任务优先级管理
 *       - 软件定时器功能
 *       - 任务状态监控
 *       - 超时检测和看门狗
 *       - 低功耗支持
 *       - 任务统计信息
 *
 * @note 设计理念:
 *       所有应用层代码都应该以任务的形式注册到调度器中执行
 *       调度器负责按优先级和时间要求调度各个任务
 */

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*=============================================================================
 *                              宏定义配置
 *============================================================================*/

/**
 * @brief 最大任务数量
 * @note 根据RAM大小和需求调整
 */
#define SCHEDULER_MAX_TASKS         16

/**
 * @brief 最大软件定时器数量
 */
#define SCHEDULER_MAX_TIMERS        8

/**
 * @brief 时基周期 (ms)
 * @note 调度器的最小时间粒度
 */
#define SCHEDULER_TICK_MS           1

/**
 * @brief 启用任务统计
 */
#define SCHEDULER_ENABLE_STATS      1

/**
 * @brief 启用看门狗
 */
#define SCHEDULER_ENABLE_WATCHDOG   1

/**
 * @brief 看门狗超时时间 (ms)
 */
#define SCHEDULER_WATCHDOG_TIMEOUT  5000

/**
 * @brief 启用空闲钩子
 */
#define SCHEDULER_ENABLE_IDLE_HOOK  1

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/**
 * @brief 任务ID类型
 */
typedef uint8_t task_id_t;

/**
 * @brief 定时器ID类型
 */
typedef uint8_t timer_id_t;

/**
 * @brief 无效ID
 */
#define INVALID_ID                  0xFF

/**
 * @brief 任务优先级
 */
typedef enum {
    TASK_PRIORITY_IDLE = 0,     /**< 空闲优先级 (最低) */
    TASK_PRIORITY_LOW,          /**< 低优先级 */
    TASK_PRIORITY_NORMAL,       /**< 普通优先级 */
    TASK_PRIORITY_HIGH,         /**< 高优先级 */
    TASK_PRIORITY_REALTIME,     /**< 实时优先级 (最高) */
    TASK_PRIORITY_COUNT
} task_priority_t;

/**
 * @brief 任务状态
 */
typedef enum {
    TASK_STATE_INVALID = 0,     /**< 无效/未使用 */
    TASK_STATE_READY,           /**< 就绪 */
    TASK_STATE_RUNNING,         /**< 运行中 */
    TASK_STATE_SUSPENDED,       /**< 挂起 */
    TASK_STATE_BLOCKED          /**< 阻塞 (等待事件) */
} task_state_t;

/**
 * @brief 任务类型
 */
typedef enum {
    TASK_TYPE_ONESHOT,          /**< 一次性任务 */
    TASK_TYPE_PERIODIC          /**< 周期性任务 */
} task_type_t;

/**
 * @brief 任务函数类型
 * @param arg 任务参数
 */
typedef void (*task_func_t)(void *arg);

/**
 * @brief 定时器回调类型
 * @param timer_id 定时器ID
 * @param arg 用户参数
 */
typedef void (*timer_callback_t)(timer_id_t timer_id, void *arg);

/**
 * @brief 空闲钩子类型
 */
typedef void (*idle_hook_t)(void);

/**
 * @brief 看门狗超时回调类型
 * @param task_id 超时的任务ID
 */
typedef void (*watchdog_callback_t)(task_id_t task_id);

/**
 * @brief 任务配置结构体
 */
typedef struct {
    const char *name;           /**< 任务名称 */
    task_func_t func;           /**< 任务函数 */
    void *arg;                  /**< 任务参数 */
    task_priority_t priority;   /**< 任务优先级 */
    task_type_t type;           /**< 任务类型 */
    uint32_t period_ms;         /**< 周期 (ms), 0表示一次性任务 */
    uint32_t delay_ms;          /**< 首次执行延迟 (ms) */
} task_config_t;

/**
 * @brief 任务统计信息
 */
typedef struct {
    uint32_t run_count;         /**< 执行次数 */
    uint32_t total_time_us;     /**< 总执行时间 (us) */
    uint32_t max_time_us;       /**< 最大执行时间 (us) */
    uint32_t avg_time_us;       /**< 平均执行时间 (us) */
    uint32_t last_run_tick;     /**< 上次执行时间 */
    uint32_t overrun_count;     /**< 超时次数 */
} task_stats_t;

/**
 * @brief 任务控制块
 */
typedef struct {
    task_config_t config;       /**< 任务配置 */
    task_state_t state;         /**< 任务状态 */
    uint32_t next_run_tick;     /**< 下次执行时间 */
    uint32_t deadline_tick;     /**< 截止时间 (看门狗) */
#if SCHEDULER_ENABLE_STATS
    task_stats_t stats;         /**< 统计信息 */
#endif
} task_tcb_t;

/**
 * @brief 软件定时器结构体
 */
typedef struct {
    uint8_t is_active;          /**< 激活状态 */
    uint8_t is_periodic;        /**< 是否周期性 */
    uint32_t period_ms;         /**< 周期/延时 (ms) */
    uint32_t expire_tick;       /**< 到期时间 */
    timer_callback_t callback;  /**< 回调函数 */
    void *arg;                  /**< 用户参数 */
} soft_timer_t;

/**
 * @brief 调度器状态
 */
typedef struct {
    uint8_t is_running;         /**< 运行状态 */
    uint32_t tick_count;        /**< 时基计数 */
    uint8_t task_count;         /**< 任务数量 */
    uint8_t timer_count;        /**< 定时器数量 */
    task_id_t current_task;     /**< 当前运行任务 */
    uint32_t idle_count;        /**< 空闲计数 */
    float cpu_usage;            /**< CPU占用率 (%) */
} scheduler_state_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/*----------------------- 调度器核心函数 -----------------------*/

/**
 * @brief 调度器初始化
 * @retval 0:成功 -1:失败
 */
int scheduler_init(void);

/**
 * @brief 调度器反初始化
 */
void scheduler_deinit(void);

/**
 * @brief 启动调度器
 * @note 此函数不会返回（进入主循环）
 */
void scheduler_start(void);

/**
 * @brief 停止调度器
 */
void scheduler_stop(void);

/**
 * @brief 调度器运行一次
 * @note 如果不使用scheduler_start()，可在主循环中调用此函数
 */
void scheduler_run(void);

/**
 * @brief 时基更新 (在SysTick中断中调用)
 */
void scheduler_tick(void);

/**
 * @brief 获取当前tick计数
 * @retval 当前tick值
 */
uint32_t scheduler_get_tick(void);

/**
 * @brief 获取调度器状态
 * @retval 调度器状态指针
 */
const scheduler_state_t* scheduler_get_state(void);

/*----------------------- 任务管理函数 -----------------------*/

/**
 * @brief 创建任务
 * @param config 任务配置
 * @retval 任务ID，失败返回INVALID_ID
 */
task_id_t scheduler_task_create(const task_config_t *config);

/**
 * @brief 删除任务
 * @param task_id 任务ID
 * @retval 0:成功 -1:失败
 */
int scheduler_task_delete(task_id_t task_id);

/**
 * @brief 挂起任务
 * @param task_id 任务ID
 * @retval 0:成功 -1:失败
 */
int scheduler_task_suspend(task_id_t task_id);

/**
 * @brief 恢复任务
 * @param task_id 任务ID
 * @retval 0:成功 -1:失败
 */
int scheduler_task_resume(task_id_t task_id);

/**
 * @brief 设置任务周期
 * @param task_id 任务ID
 * @param period_ms 新周期 (ms)
 * @retval 0:成功 -1:失败
 */
int scheduler_task_set_period(task_id_t task_id, uint32_t period_ms);

/**
 * @brief 设置任务优先级
 * @param task_id 任务ID
 * @param priority 新优先级
 * @retval 0:成功 -1:失败
 */
int scheduler_task_set_priority(task_id_t task_id, task_priority_t priority);

/**
 * @brief 获取任务状态
 * @param task_id 任务ID
 * @retval 任务状态
 */
task_state_t scheduler_task_get_state(task_id_t task_id);

/**
 * @brief 获取任务统计信息
 * @param task_id 任务ID
 * @retval 任务统计信息指针，失败返回NULL
 */
const task_stats_t* scheduler_task_get_stats(task_id_t task_id);

/**
 * @brief 通过名称查找任务ID
 * @param name 任务名称
 * @retval 任务ID，未找到返回INVALID_ID
 */
task_id_t scheduler_task_find(const char *name);

/**
 * @brief 重置任务统计信息
 * @param task_id 任务ID
 */
void scheduler_task_reset_stats(task_id_t task_id);

/*----------------------- 软件定时器函数 -----------------------*/

/**
 * @brief 创建软件定时器
 * @param period_ms 定时周期 (ms)
 * @param callback 回调函数
 * @param arg 用户参数
 * @param is_periodic 是否周期性
 * @retval 定时器ID，失败返回INVALID_ID
 */
timer_id_t scheduler_timer_create(uint32_t period_ms,
                                   timer_callback_t callback,
                                   void *arg,
                                   uint8_t is_periodic);

/**
 * @brief 删除软件定时器
 * @param timer_id 定时器ID
 * @retval 0:成功 -1:失败
 */
int scheduler_timer_delete(timer_id_t timer_id);

/**
 * @brief 启动软件定时器
 * @param timer_id 定时器ID
 * @retval 0:成功 -1:失败
 */
int scheduler_timer_start(timer_id_t timer_id);

/**
 * @brief 停止软件定时器
 * @param timer_id 定时器ID
 * @retval 0:成功 -1:失败
 */
int scheduler_timer_stop(timer_id_t timer_id);

/**
 * @brief 重置软件定时器
 * @param timer_id 定时器ID
 * @retval 0:成功 -1:失败
 */
int scheduler_timer_reset(timer_id_t timer_id);

/**
 * @brief 修改定时器周期
 * @param timer_id 定时器ID
 * @param period_ms 新周期 (ms)
 * @retval 0:成功 -1:失败
 */
int scheduler_timer_set_period(timer_id_t timer_id, uint32_t period_ms);

/*----------------------- 延时函数 -----------------------*/

/**
 * @brief 非阻塞延时 (协作式)
 * @param ms 延时时间 (ms)
 * @note 此函数会让出CPU给其他任务
 */
void scheduler_delay(uint32_t ms);

/**
 * @brief 阻塞延时
 * @param ms 延时时间 (ms)
 * @note 此函数不会让出CPU
 */
void scheduler_delay_blocking(uint32_t ms);

/*----------------------- 钩子函数 -----------------------*/

/**
 * @brief 设置空闲钩子
 * @param hook 钩子函数
 */
void scheduler_set_idle_hook(idle_hook_t hook);

/**
 * @brief 设置看门狗回调
 * @param callback 回调函数
 */
void scheduler_set_watchdog_callback(watchdog_callback_t callback);

/*----------------------- 同步原语 -----------------------*/

/**
 * @brief 进入临界区
 */
void scheduler_enter_critical(void);

/**
 * @brief 退出临界区
 */
void scheduler_exit_critical(void);

/*----------------------- 辅助函数 -----------------------*/

/**
 * @brief 获取系统运行时间 (ms)
 * @retval 运行时间 (ms)
 */
uint32_t scheduler_get_runtime_ms(void);

/**
 * @brief 获取CPU占用率
 * @retval CPU占用率 (0-100%)
 */
float scheduler_get_cpu_usage(void);

/**
 * @brief 打印任务列表 (调试用)
 * @param print_func 打印函数
 */
void scheduler_print_tasks(void (*print_func)(const char *));

/*=============================================================================
 *                              便捷宏定义
 *============================================================================*/

/**
 * @brief 快速创建周期任务
 */
#define TASK_PERIODIC(name, func, period, prio) \
    { .name = name, .func = func, .arg = NULL, \
      .priority = prio, .type = TASK_TYPE_PERIODIC, \
      .period_ms = period, .delay_ms = 0 }

/**
 * @brief 快速创建一次性任务
 */
#define TASK_ONESHOT(name, func, delay, prio) \
    { .name = name, .func = func, .arg = NULL, \
      .priority = prio, .type = TASK_TYPE_ONESHOT, \
      .period_ms = 0, .delay_ms = delay }

/**
 * @brief 快速创建带参数的周期任务
 */
#define TASK_PERIODIC_ARG(name, func, arg, period, prio) \
    { .name = name, .func = func, .arg = arg, \
      .priority = prio, .type = TASK_TYPE_PERIODIC, \
      .period_ms = period, .delay_ms = 0 }

#ifdef __cplusplus
}
#endif

#endif /* __SCHEDULER_H */
