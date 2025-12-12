/**
 * @file scheduler.c
 * @brief 伪调度器模块实现 - 协作式任务调度系统
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "scheduler.h"
#include <string.h>
#include <stdio.h>

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static task_tcb_t task_list[SCHEDULER_MAX_TASKS];
static soft_timer_t timer_list[SCHEDULER_MAX_TIMERS];
static scheduler_state_t scheduler_state = {0};

static idle_hook_t idle_hook = NULL;
static watchdog_callback_t watchdog_cb = NULL;

static volatile uint32_t tick_count = 0;
static volatile uint8_t critical_nesting = 0;

/* CPU占用率计算 */
static uint32_t idle_start_tick = 0;
static uint32_t busy_time = 0;
static uint32_t sample_start_tick = 0;

/*=============================================================================
 *                              外部函数声明 (需用户实现)
 *============================================================================*/

/**
 * @brief 获取当前时间戳 (us)
 * @note 用于任务执行时间统计，用户可根据硬件实现
 */
__attribute__((weak)) uint32_t scheduler_get_us(void)
{
    return tick_count * 1000;
}

/**
 * @brief 禁用中断
 */
__attribute__((weak)) void scheduler_disable_irq(void)
{
    __asm volatile("cpsid i");
}

/**
 * @brief 使能中断
 */
__attribute__((weak)) void scheduler_enable_irq(void)
{
    __asm volatile("cpsie i");
}

/**
 * @brief 喂狗函数
 */
__attribute__((weak)) void scheduler_feed_watchdog(void)
{
    /* 用户实现硬件看门狗喂狗 */
}

/**
 * @brief 进入低功耗模式
 */
__attribute__((weak)) void scheduler_enter_sleep(void)
{
    __asm volatile("wfi");
}

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static task_id_t find_free_task_slot(void);
static timer_id_t find_free_timer_slot(void);
static void process_timers(void);
static void check_watchdog(void);
static void update_cpu_usage(void);

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief 调度器初始化
 */
int scheduler_init(void)
{
    uint8_t i;

    /* 清空任务列表 */
    memset(task_list, 0, sizeof(task_list));
    for (i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        task_list[i].state = TASK_STATE_INVALID;
    }

    /* 清空定时器列表 */
    memset(timer_list, 0, sizeof(timer_list));

    /* 初始化调度器状态 */
    memset(&scheduler_state, 0, sizeof(scheduler_state));
    scheduler_state.current_task = INVALID_ID;

    tick_count = 0;
    critical_nesting = 0;
    idle_start_tick = 0;
    busy_time = 0;
    sample_start_tick = 0;

    return 0;
}

/**
 * @brief 调度器反初始化
 */
void scheduler_deinit(void)
{
    scheduler_stop();
    memset(task_list, 0, sizeof(task_list));
    memset(timer_list, 0, sizeof(timer_list));
}

/**
 * @brief 启动调度器 (进入主循环)
 */
void scheduler_start(void)
{
    scheduler_state.is_running = 1;
    sample_start_tick = tick_count;

    while (scheduler_state.is_running) {
        scheduler_run();
    }
}

/**
 * @brief 停止调度器
 */
void scheduler_stop(void)
{
    scheduler_state.is_running = 0;
}

/**
 * @brief 调度器运行一次
 */
void scheduler_run(void)
{
    uint8_t i;
    task_id_t highest_prio_task = INVALID_ID;
    task_priority_t highest_prio = TASK_PRIORITY_IDLE;
    uint32_t current_tick = tick_count;
    uint8_t task_executed = 0;

#if SCHEDULER_ENABLE_STATS
    uint32_t start_us, end_us;
#endif

    /* 处理软件定时器 */
    process_timers();

    /* 查找最高优先级就绪任务 */
    for (i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        if (task_list[i].state == TASK_STATE_READY) {
            /* 检查是否到执行时间 */
            if ((int32_t)(current_tick - task_list[i].next_run_tick) >= 0) {
                if (task_list[i].config.priority > highest_prio ||
                    highest_prio_task == INVALID_ID) {
                    highest_prio = task_list[i].config.priority;
                    highest_prio_task = i;
                }
            }
        }
    }

    /* 执行任务 */
    if (highest_prio_task != INVALID_ID) {
        task_tcb_t *tcb = &task_list[highest_prio_task];

        /* 更新状态 */
        tcb->state = TASK_STATE_RUNNING;
        scheduler_state.current_task = highest_prio_task;

#if SCHEDULER_ENABLE_STATS
        start_us = scheduler_get_us();
#endif

        /* 执行任务函数 */
        if (tcb->config.func != NULL) {
            tcb->config.func(tcb->config.arg);
        }

#if SCHEDULER_ENABLE_STATS
        end_us = scheduler_get_us();
        {
            uint32_t exec_time = end_us - start_us;
            tcb->stats.run_count++;
            tcb->stats.total_time_us += exec_time;
            if (exec_time > tcb->stats.max_time_us) {
                tcb->stats.max_time_us = exec_time;
            }
            tcb->stats.avg_time_us = tcb->stats.total_time_us / tcb->stats.run_count;
            tcb->stats.last_run_tick = current_tick;

            /* 检查是否超时 */
            if (tcb->config.type == TASK_TYPE_PERIODIC &&
                exec_time > tcb->config.period_ms * 1000) {
                tcb->stats.overrun_count++;
            }
        }
#endif

        /* 更新下次执行时间 */
        if (tcb->config.type == TASK_TYPE_PERIODIC) {
            tcb->next_run_tick = current_tick + tcb->config.period_ms;
            tcb->state = TASK_STATE_READY;
#if SCHEDULER_ENABLE_WATCHDOG
            tcb->deadline_tick = tcb->next_run_tick + SCHEDULER_WATCHDOG_TIMEOUT;
#endif
        } else {
            /* 一次性任务执行完毕后删除 */
            tcb->state = TASK_STATE_INVALID;
            scheduler_state.task_count--;
        }

        scheduler_state.current_task = INVALID_ID;
        task_executed = 1;
    }

    /* 空闲处理 */
    if (!task_executed) {
        scheduler_state.idle_count++;

        /* 调用空闲钩子 */
#if SCHEDULER_ENABLE_IDLE_HOOK
        if (idle_hook != NULL) {
            idle_hook();
        }
#endif

        /* 喂狗 */
        scheduler_feed_watchdog();

        /* 进入低功耗 (可选) */
        /* scheduler_enter_sleep(); */
    } else {
        busy_time++;
    }

    /* 更新CPU占用率 */
    update_cpu_usage();

#if SCHEDULER_ENABLE_WATCHDOG
    /* 检查看门狗 */
    check_watchdog();
#endif
}

/**
 * @brief 时基更新
 */
void scheduler_tick(void)
{
    tick_count++;
    scheduler_state.tick_count = tick_count;
}

/**
 * @brief 获取当前tick计数
 */
uint32_t scheduler_get_tick(void)
{
    return tick_count;
}

/**
 * @brief 获取调度器状态
 */
const scheduler_state_t* scheduler_get_state(void)
{
    return &scheduler_state;
}

/**
 * @brief 创建任务
 */
task_id_t scheduler_task_create(const task_config_t *config)
{
    task_id_t id;
    task_tcb_t *tcb;

    if (config == NULL || config->func == NULL) {
        return INVALID_ID;
    }

    /* 查找空闲槽位 */
    id = find_free_task_slot();
    if (id == INVALID_ID) {
        return INVALID_ID;
    }

    tcb = &task_list[id];

    /* 填充任务控制块 */
    tcb->config = *config;
    tcb->state = TASK_STATE_READY;
    tcb->next_run_tick = tick_count + config->delay_ms;

#if SCHEDULER_ENABLE_STATS
    memset(&tcb->stats, 0, sizeof(tcb->stats));
#endif

#if SCHEDULER_ENABLE_WATCHDOG
    if (config->type == TASK_TYPE_PERIODIC) {
        tcb->deadline_tick = tcb->next_run_tick + SCHEDULER_WATCHDOG_TIMEOUT;
    }
#endif

    scheduler_state.task_count++;

    return id;
}

/**
 * @brief 删除任务
 */
int scheduler_task_delete(task_id_t task_id)
{
    if (task_id >= SCHEDULER_MAX_TASKS) {
        return -1;
    }

    if (task_list[task_id].state == TASK_STATE_INVALID) {
        return -1;
    }

    task_list[task_id].state = TASK_STATE_INVALID;
    scheduler_state.task_count--;

    return 0;
}

/**
 * @brief 挂起任务
 */
int scheduler_task_suspend(task_id_t task_id)
{
    if (task_id >= SCHEDULER_MAX_TASKS) {
        return -1;
    }

    if (task_list[task_id].state == TASK_STATE_INVALID) {
        return -1;
    }

    task_list[task_id].state = TASK_STATE_SUSPENDED;
    return 0;
}

/**
 * @brief 恢复任务
 */
int scheduler_task_resume(task_id_t task_id)
{
    if (task_id >= SCHEDULER_MAX_TASKS) {
        return -1;
    }

    if (task_list[task_id].state != TASK_STATE_SUSPENDED) {
        return -1;
    }

    task_list[task_id].state = TASK_STATE_READY;
    task_list[task_id].next_run_tick = tick_count;
    return 0;
}

/**
 * @brief 设置任务周期
 */
int scheduler_task_set_period(task_id_t task_id, uint32_t period_ms)
{
    if (task_id >= SCHEDULER_MAX_TASKS) {
        return -1;
    }

    if (task_list[task_id].state == TASK_STATE_INVALID) {
        return -1;
    }

    task_list[task_id].config.period_ms = period_ms;
    return 0;
}

/**
 * @brief 设置任务优先级
 */
int scheduler_task_set_priority(task_id_t task_id, task_priority_t priority)
{
    if (task_id >= SCHEDULER_MAX_TASKS || priority >= TASK_PRIORITY_COUNT) {
        return -1;
    }

    if (task_list[task_id].state == TASK_STATE_INVALID) {
        return -1;
    }

    task_list[task_id].config.priority = priority;
    return 0;
}

/**
 * @brief 获取任务状态
 */
task_state_t scheduler_task_get_state(task_id_t task_id)
{
    if (task_id >= SCHEDULER_MAX_TASKS) {
        return TASK_STATE_INVALID;
    }

    return task_list[task_id].state;
}

/**
 * @brief 获取任务统计信息
 */
const task_stats_t* scheduler_task_get_stats(task_id_t task_id)
{
#if SCHEDULER_ENABLE_STATS
    if (task_id >= SCHEDULER_MAX_TASKS) {
        return NULL;
    }

    if (task_list[task_id].state == TASK_STATE_INVALID) {
        return NULL;
    }

    return &task_list[task_id].stats;
#else
    (void)task_id;
    return NULL;
#endif
}

/**
 * @brief 通过名称查找任务ID
 */
task_id_t scheduler_task_find(const char *name)
{
    uint8_t i;

    if (name == NULL) {
        return INVALID_ID;
    }

    for (i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        if (task_list[i].state != TASK_STATE_INVALID &&
            task_list[i].config.name != NULL &&
            strcmp(task_list[i].config.name, name) == 0) {
            return i;
        }
    }

    return INVALID_ID;
}

/**
 * @brief 重置任务统计信息
 */
void scheduler_task_reset_stats(task_id_t task_id)
{
#if SCHEDULER_ENABLE_STATS
    if (task_id < SCHEDULER_MAX_TASKS &&
        task_list[task_id].state != TASK_STATE_INVALID) {
        memset(&task_list[task_id].stats, 0, sizeof(task_stats_t));
    }
#else
    (void)task_id;
#endif
}

/**
 * @brief 创建软件定时器
 */
timer_id_t scheduler_timer_create(uint32_t period_ms,
                                   timer_callback_t callback,
                                   void *arg,
                                   uint8_t is_periodic)
{
    timer_id_t id;

    if (callback == NULL || period_ms == 0) {
        return INVALID_ID;
    }

    id = find_free_timer_slot();
    if (id == INVALID_ID) {
        return INVALID_ID;
    }

    timer_list[id].period_ms = period_ms;
    timer_list[id].callback = callback;
    timer_list[id].arg = arg;
    timer_list[id].is_periodic = is_periodic;
    timer_list[id].is_active = 0;
    timer_list[id].expire_tick = 0;

    scheduler_state.timer_count++;

    return id;
}

/**
 * @brief 删除软件定时器
 */
int scheduler_timer_delete(timer_id_t timer_id)
{
    if (timer_id >= SCHEDULER_MAX_TIMERS) {
        return -1;
    }

    if (timer_list[timer_id].callback == NULL) {
        return -1;
    }

    memset(&timer_list[timer_id], 0, sizeof(soft_timer_t));
    scheduler_state.timer_count--;

    return 0;
}

/**
 * @brief 启动软件定时器
 */
int scheduler_timer_start(timer_id_t timer_id)
{
    if (timer_id >= SCHEDULER_MAX_TIMERS) {
        return -1;
    }

    if (timer_list[timer_id].callback == NULL) {
        return -1;
    }

    timer_list[timer_id].expire_tick = tick_count + timer_list[timer_id].period_ms;
    timer_list[timer_id].is_active = 1;

    return 0;
}

/**
 * @brief 停止软件定时器
 */
int scheduler_timer_stop(timer_id_t timer_id)
{
    if (timer_id >= SCHEDULER_MAX_TIMERS) {
        return -1;
    }

    timer_list[timer_id].is_active = 0;

    return 0;
}

/**
 * @brief 重置软件定时器
 */
int scheduler_timer_reset(timer_id_t timer_id)
{
    if (timer_id >= SCHEDULER_MAX_TIMERS) {
        return -1;
    }

    if (timer_list[timer_id].callback == NULL) {
        return -1;
    }

    timer_list[timer_id].expire_tick = tick_count + timer_list[timer_id].period_ms;

    return 0;
}

/**
 * @brief 修改定时器周期
 */
int scheduler_timer_set_period(timer_id_t timer_id, uint32_t period_ms)
{
    if (timer_id >= SCHEDULER_MAX_TIMERS || period_ms == 0) {
        return -1;
    }

    timer_list[timer_id].period_ms = period_ms;

    return 0;
}

/**
 * @brief 非阻塞延时
 */
void scheduler_delay(uint32_t ms)
{
    uint32_t start = tick_count;

    while ((tick_count - start) < ms) {
        scheduler_run();
    }
}

/**
 * @brief 阻塞延时
 */
void scheduler_delay_blocking(uint32_t ms)
{
    uint32_t start = tick_count;

    while ((tick_count - start) < ms) {
        /* 空等待 */
    }
}

/**
 * @brief 设置空闲钩子
 */
void scheduler_set_idle_hook(idle_hook_t hook)
{
    idle_hook = hook;
}

/**
 * @brief 设置看门狗回调
 */
void scheduler_set_watchdog_callback(watchdog_callback_t callback)
{
    watchdog_cb = callback;
}

/**
 * @brief 进入临界区
 */
void scheduler_enter_critical(void)
{
    scheduler_disable_irq();
    critical_nesting++;
}

/**
 * @brief 退出临界区
 */
void scheduler_exit_critical(void)
{
    critical_nesting--;
    if (critical_nesting == 0) {
        scheduler_enable_irq();
    }
}

/**
 * @brief 获取系统运行时间
 */
uint32_t scheduler_get_runtime_ms(void)
{
    return tick_count * SCHEDULER_TICK_MS;
}

/**
 * @brief 获取CPU占用率
 */
float scheduler_get_cpu_usage(void)
{
    return scheduler_state.cpu_usage;
}

/**
 * @brief 打印任务列表
 */
void scheduler_print_tasks(void (*print_func)(const char *))
{
    uint8_t i;
    char buf[128];
    static const char *state_str[] = {"INV", "RDY", "RUN", "SUS", "BLK"};
    static const char *prio_str[] = {"IDLE", "LOW", "NORM", "HIGH", "RT"};

    if (print_func == NULL) {
        return;
    }

    print_func("=== Task List ===\n");
    snprintf(buf, sizeof(buf), "Total: %d, CPU: %.1f%%\n",
             scheduler_state.task_count, scheduler_state.cpu_usage);
    print_func(buf);

    print_func("ID  Name            State  Prio  Period  RunCnt  AvgUs\n");
    print_func("--  --------------  -----  ----  ------  ------  -----\n");

    for (i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        if (task_list[i].state != TASK_STATE_INVALID) {
            snprintf(buf, sizeof(buf),
                     "%2d  %-14s  %-5s  %-4s  %6lu  %6lu  %5lu\n",
                     i,
                     task_list[i].config.name ? task_list[i].config.name : "(null)",
                     state_str[task_list[i].state],
                     prio_str[task_list[i].config.priority],
                     (unsigned long)task_list[i].config.period_ms,
#if SCHEDULER_ENABLE_STATS
                     (unsigned long)task_list[i].stats.run_count,
                     (unsigned long)task_list[i].stats.avg_time_us
#else
                     0UL, 0UL
#endif
                    );
            print_func(buf);
        }
    }
}

/*=============================================================================
 *                              私有函数实现
 *============================================================================*/

/**
 * @brief 查找空闲任务槽位
 */
static task_id_t find_free_task_slot(void)
{
    uint8_t i;

    for (i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        if (task_list[i].state == TASK_STATE_INVALID) {
            return i;
        }
    }

    return INVALID_ID;
}

/**
 * @brief 查找空闲定时器槽位
 */
static timer_id_t find_free_timer_slot(void)
{
    uint8_t i;

    for (i = 0; i < SCHEDULER_MAX_TIMERS; i++) {
        if (timer_list[i].callback == NULL) {
            return i;
        }
    }

    return INVALID_ID;
}

/**
 * @brief 处理软件定时器
 */
static void process_timers(void)
{
    uint8_t i;
    uint32_t current_tick = tick_count;

    for (i = 0; i < SCHEDULER_MAX_TIMERS; i++) {
        if (timer_list[i].is_active &&
            timer_list[i].callback != NULL) {

            if ((int32_t)(current_tick - timer_list[i].expire_tick) >= 0) {
                /* 调用回调 */
                timer_list[i].callback(i, timer_list[i].arg);

                if (timer_list[i].is_periodic) {
                    /* 重新计算下次到期时间 */
                    timer_list[i].expire_tick = current_tick + timer_list[i].period_ms;
                } else {
                    /* 一次性定时器，停止 */
                    timer_list[i].is_active = 0;
                }
            }
        }
    }
}

/**
 * @brief 检查看门狗
 */
static void check_watchdog(void)
{
#if SCHEDULER_ENABLE_WATCHDOG
    uint8_t i;
    uint32_t current_tick = tick_count;

    for (i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        if (task_list[i].state == TASK_STATE_READY &&
            task_list[i].config.type == TASK_TYPE_PERIODIC) {

            if ((int32_t)(current_tick - task_list[i].deadline_tick) > 0) {
                /* 任务超时 */
                if (watchdog_cb != NULL) {
                    watchdog_cb(i);
                }

                /* 重置截止时间 */
                task_list[i].deadline_tick = current_tick + SCHEDULER_WATCHDOG_TIMEOUT;
            }
        }
    }
#endif
}

/**
 * @brief 更新CPU占用率
 */
static void update_cpu_usage(void)
{
    uint32_t sample_time = tick_count - sample_start_tick;

    /* 每1000ms计算一次CPU占用率 */
    if (sample_time >= 1000) {
        scheduler_state.cpu_usage = (float)busy_time * 100.0f / (float)sample_time;
        busy_time = 0;
        sample_start_tick = tick_count;
    }
}
