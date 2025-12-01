/**
  ******************************************************************************
  * @file    bsp_ec11.c
  * @author  TFT_EC11_KEY Project
  * @brief   EC11旋转编码器驱动实现 - STM32F407 标准库版本
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_ec11.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "misc.h"

/* Private variables ---------------------------------------------------------*/
static ec11_state_t ec11_state = {0};
static ec11_event_callback_t event_callback = NULL;
static volatile uint32_t last_exti_time = 0;  /* 用于消抖 */

/* 弱定义系统时间戳函数，用户需要在外部重新实现 ------------------------------*/
__weak uint32_t bsp_ec11_get_tick(void)
{
    /* 默认返回0，用户需要根据自己的系统实现 */
    /* 例如：return HAL_GetTick(); 或者 return SysTick毫秒计数 */
    return 0;
}

/* Private function prototypes -----------------------------------------------*/
static void ec11_gpio_config(void);
static void ec11_exti_config(void);
static void ec11_nvic_config(void);
static uint8_t ec11_read_pin_a(void);
static uint8_t ec11_read_pin_b(void);
static uint8_t ec11_read_pin_key(void);

/**
  * @brief  EC11编码器初始化
  * @param  None
  * @retval None
  */
void bsp_ec11_init(void)
{
    /* 配置GPIO */
    ec11_gpio_config();

    /* 配置外部中断 */
    ec11_exti_config();

    /* 配置NVIC */
    ec11_nvic_config();

    /* 初始化状态 */
    ec11_state.count = 0;
    ec11_state.key_state = 0;
    ec11_state.key_press_time = 0;
    ec11_state.last_a_state = ec11_read_pin_a();
    ec11_state.last_b_state = ec11_read_pin_b();
}

/**
  * @brief  EC11 GPIO配置
  * @param  None
  * @retval None
  */
static void ec11_gpio_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIO时钟 */
    RCC_AHB1PeriphClockCmd(EC11_A_GPIO_CLK | EC11_B_GPIO_CLK | EC11_KEY_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* 配置EC11 A相引脚 - 上拉输入 */
    GPIO_InitStructure.GPIO_Pin = EC11_A_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(EC11_A_GPIO_PORT, &GPIO_InitStructure);

    /* 配置EC11 B相引脚 - 上拉输入 */
    GPIO_InitStructure.GPIO_Pin = EC11_B_GPIO_PIN;
    GPIO_Init(EC11_B_GPIO_PORT, &GPIO_InitStructure);

    /* 配置EC11按键引脚 - 上拉输入 */
    GPIO_InitStructure.GPIO_Pin = EC11_KEY_GPIO_PIN;
    GPIO_Init(EC11_KEY_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * @brief  EC11外部中断配置
  * @param  None
  * @retval None
  */
static void ec11_exti_config(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* 连接EXTI线到GPIO引脚 */
    SYSCFG_EXTILineConfig(EC11_A_EXTI_PORT_SOURCE, EC11_A_EXTI_PIN_SOURCE);
    SYSCFG_EXTILineConfig(EC11_KEY_EXTI_PORT_SOURCE, EC11_KEY_EXTI_PIN_SOURCE);

    /* 配置EXTI线 - EC11 A相 (下降沿触发) */
    EXTI_InitStructure.EXTI_Line = EC11_A_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* 配置EXTI线 - EC11按键 (下降沿触发) */
    EXTI_InitStructure.EXTI_Line = EC11_KEY_EXTI_LINE;
    EXTI_Init(&EXTI_InitStructure);
}

/**
  * @brief  EC11 NVIC配置
  * @param  None
  * @retval None
  */
static void ec11_nvic_config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 配置NVIC优先级分组 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 配置EXTI0中断 - EC11 A相 */
    NVIC_InitStructure.NVIC_IRQChannel = EC11_A_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 配置EXTI2中断 - EC11按键 */
    NVIC_InitStructure.NVIC_IRQChannel = EC11_KEY_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  读取EC11 A相引脚状态
  * @param  None
  * @retval 引脚状态: 0或1
  */
static uint8_t ec11_read_pin_a(void)
{
    return GPIO_ReadInputDataBit(EC11_A_GPIO_PORT, EC11_A_GPIO_PIN);
}

/**
  * @brief  读取EC11 B相引脚状态
  * @param  None
  * @retval 引脚状态: 0或1
  */
static uint8_t ec11_read_pin_b(void)
{
    return GPIO_ReadInputDataBit(EC11_B_GPIO_PORT, EC11_B_GPIO_PIN);
}

/**
  * @brief  读取EC11按键引脚状态
  * @param  None
  * @retval 引脚状态: 0-按下, 1-释放
  */
static uint8_t ec11_read_pin_key(void)
{
    return GPIO_ReadInputDataBit(EC11_KEY_GPIO_PORT, EC11_KEY_GPIO_PIN);
}

/**
  * @brief  EC11扫描函数 (需要周期调用)
  * @param  None
  * @retval 当前事件
  */
ec11_event_t bsp_ec11_scan(void)
{
    ec11_event_t event = EC11_EVENT_NONE;
    uint32_t current_time = bsp_ec11_get_tick();
    uint8_t key_pin = ec11_read_pin_key();

    /* 按键状态检测 */
    if (key_pin == 0) {  /* 按键按下 (低电平) */
        if (ec11_state.key_state == 0) {
            /* 按键刚按下 */
            ec11_state.key_state = 1;
            ec11_state.key_press_time = current_time;
        } else {
            /* 按键保持按下 - 检查长按 */
            if ((current_time - ec11_state.key_press_time) >= EC11_LONG_PRESS_TIME_MS) {
                event = EC11_EVENT_KEY_LONG_PRESS;
                ec11_state.key_press_time = current_time;  /* 重置，避免重复触发 */
            }
        }
    } else {  /* 按键释放 */
        if (ec11_state.key_state == 1) {
            /* 按键刚释放 */
            uint32_t press_duration = current_time - ec11_state.key_press_time;

            if (press_duration < EC11_LONG_PRESS_TIME_MS) {
                /* 短按 */
                event = EC11_EVENT_KEY_SHORT_PRESS;
            }

            ec11_state.key_state = 0;
        }
    }

    /* 触发回调 */
    if (event != EC11_EVENT_NONE && event_callback != NULL) {
        event_callback(event);
    }

    return event;
}

/**
  * @brief  EXTI中断回调函数
  * @param  exti_line: 中断线
  * @retval None
  */
void bsp_ec11_exti_callback(uint32_t exti_line)
{
    uint32_t current_time = bsp_ec11_get_tick();
    ec11_event_t event = EC11_EVENT_NONE;

    /* 软件消抖 */
    if ((current_time - last_exti_time) < EC11_DEBOUNCE_TIME_MS) {
        return;
    }
    last_exti_time = current_time;

    /* EC11 A相中断 - 检测旋转方向 */
    if (exti_line == EC11_A_EXTI_LINE) {
        uint8_t a_state = ec11_read_pin_a();
        uint8_t b_state = ec11_read_pin_b();

        /* EC11编码器正交解码 */
        if (a_state == 0) {  /* A相下降沿 */
            if (b_state == 1) {
                /* 顺时针旋转 (右旋) */
                ec11_state.count++;
                event = EC11_EVENT_ROTATE_RIGHT;
            } else {
                /* 逆时针旋转 (左旋) */
                ec11_state.count--;
                event = EC11_EVENT_ROTATE_LEFT;
            }
        }

        ec11_state.last_a_state = a_state;
        ec11_state.last_b_state = b_state;
    }

    /* 触发回调 */
    if (event != EC11_EVENT_NONE && event_callback != NULL) {
        event_callback(event);
    }
}

/**
  * @brief  获取EC11旋转计数值
  * @param  None
  * @retval 当前计数值
  */
int32_t bsp_ec11_get_count(void)
{
    return ec11_state.count;
}

/**
  * @brief  设置EC11旋转计数值
  * @param  count: 要设置的计数值
  * @retval None
  */
void bsp_ec11_set_count(int32_t count)
{
    ec11_state.count = count;
}

/**
  * @brief  获取EC11按键状态
  * @param  None
  * @retval 0-未按下, 1-按下
  */
uint8_t bsp_ec11_get_key_state(void)
{
    return ec11_state.key_state;
}

/**
  * @brief  注册事件回调函数
  * @param  callback: 回调函数指针
  * @retval None
  */
void bsp_ec11_register_callback(ec11_event_callback_t callback)
{
    event_callback = callback;
}

/**
  * @brief  EXTI0中断服务函数 (EC11 A相)
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EC11_A_EXTI_LINE) != RESET) {
        bsp_ec11_exti_callback(EC11_A_EXTI_LINE);
        EXTI_ClearITPendingBit(EC11_A_EXTI_LINE);
    }
}

/**
  * @brief  EXTI2中断服务函数 (EC11按键)
  * @param  None
  * @retval None
  */
void EXTI2_IRQHandler(void)
{
    if (EXTI_GetITStatus(EC11_KEY_EXTI_LINE) != RESET) {
        /* 按键事件在scan函数中处理，这里只清除中断标志 */
        EXTI_ClearITPendingBit(EC11_KEY_EXTI_LINE);
    }
}
