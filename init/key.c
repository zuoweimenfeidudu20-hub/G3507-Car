#include "key.h"
#include "ti_msp_dl_config.h"
#include "pid_motor.h"

/* 上拉输入：低电平按下。双边沿中断记录采样间的抖动，定时器确认稳定电平。 */
#define KEY_COUNT 4U
#define KEY_DEBOUNCE_TICKS 3U /* 首次采样后再稳定 3 个 10 ms 周期。 */
typedef struct { GPIO_Regs *port; uint32_t pin; } Key_Pin;
typedef struct {
    uint8_t candidate, stable, ticks, armed;
} Key_Filter;
static const Key_Pin s_pins[KEY_COUNT] = {
    {key_PIN_key1_PORT, key_PIN_key1_PIN},
    {key_PIN_key2_PORT, key_PIN_key2_PIN},
    {key_PIN_key3_PORT, key_PIN_key3_PIN},
    {key_PIN_key4_PORT, key_PIN_key4_PIN}
};
static Key_Filter s_filters[KEY_COUNT];
static volatile uint8_t s_edges;
static volatile uint8_t s_press_events;
static volatile Key_Mode s_requested_mode;
static Key_Mode s_mode;
static uint8_t s_initialized;
volatile uint32_t g_key_count;

void Key_Init(void)
{
    uint32_t i;
    s_initialized = 0U;
    s_edges = 0U;
    s_press_events = 0U;
    s_requested_mode = KEY_MODE_IDLE;
    s_mode = KEY_MODE_IDLE;
    g_key_count = 0U;
    PIDMotor_SetManualPWM(0, 0);
    for (i = 0U; i < KEY_COUNT; ++i) {
        Key_Filter *f = &s_filters[i];
        f->candidate = (DL_GPIO_readPins(s_pins[i].port, s_pins[i].pin) != 0U);
        f->stable = 2U; /* 尚未确认电平。 */
        f->ticks = 0U;
        f->armed = 0U; /* 上电按住不触发，必须先稳定松开。 */
        DL_GPIO_clearInterruptStatus(s_pins[i].port, s_pins[i].pin);
    }
    /* Only key 1 has a GPIO interrupt; the timer polls all four levels. */
    DL_GPIO_enableInterrupt(s_pins[0].port, s_pins[0].pin);
    s_initialized = 1U;
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

void Key_HandleInterrupt(void)
{
    uint32_t i;
    /* 只清除按键引脚中断，不影响共用 GROUP1 的编码器。 */
    for (i = 0U; i < KEY_COUNT; ++i) {
        if (DL_GPIO_getEnabledInterruptStatus(s_pins[i].port, s_pins[i].pin)) {
            DL_GPIO_clearInterruptStatus(s_pins[i].port, s_pins[i].pin);
            s_edges |= (uint8_t)(1U << i);
        }
    }
}

void Key_Scan10ms(void)
{
    uint32_t i, irq_state;
    uint8_t presses = 0U;
    if (s_initialized == 0U) return;
    /* 短临界区防止高优先级 GPIO ISR 的边沿标记丢失。 */
    irq_state = __get_PRIMASK();
    __disable_irq();
    Key_HandleInterrupt();
    for (i = 0U; i < KEY_COUNT; ++i) {
        Key_Filter *f = &s_filters[i];
        uint8_t level = (DL_GPIO_readPins(s_pins[i].port, s_pins[i].pin) != 0U);
        if ((level != f->candidate) || ((s_edges & (1U << i)) != 0U)) {
            f->candidate = level;
            f->ticks = 0U;
        } else if (f->ticks < KEY_DEBOUNCE_TICKS) {
            ++f->ticks;
        }
        if ((f->ticks == KEY_DEBOUNCE_TICKS) && (f->stable != level)) {
            f->stable = level;
            if (level != 0U) {
                f->armed = 1U;
            } else if (f->armed != 0U) {
                f->armed = 0U;
                presses |= (uint8_t)(1U << i);
                ++g_key_count;
            }
        }
    }
    s_edges = 0U;
    s_press_events |= presses;
    if (irq_state == 0U) __enable_irq();
    /* 同一周期多键按下：编号小的优先；主循环繁忙时仅保留最新模式请求。 */
    for (i = 0U; i < KEY_COUNT; ++i) {
        if ((presses & (1U << i)) != 0U) {
            s_requested_mode = (Key_Mode)(i + 1U);
            break;
        }
    }
}

/* Experiment_Run owns the mode behavior; this module debounces the keys. */
static void Key_ModeExit(Key_Mode mode)
{
    switch (mode) {
        case KEY_MODE_1: break; /* TODO: 模式1退出清理。 */
        case KEY_MODE_2: break;
        case KEY_MODE_3: break;
        case KEY_MODE_4: break;
        default: break;
    }
}

static void Key_ModeEnter(Key_Mode mode)
{
    switch (mode) {
        case KEY_MODE_1: break;
        case KEY_MODE_2: break;
        case KEY_MODE_3: break; /* TODO: 按键3，模式3初始化。 */
        case KEY_MODE_4: break; /* TODO: 按键4，模式4初始化。 */
        default: break;
    }
}

void Key_Task(void)
{
    Key_Mode requested = s_requested_mode;
    if (requested != s_mode) {
        Key_ModeExit(s_mode);
        s_mode = requested;
        Key_ModeEnter(s_mode);
    }
    /* 重复按当前模式键不重新初始化；长按不连发。 */
    switch (s_mode) {
        case KEY_MODE_IDLE: break; /* 上电等待按键。 */
        case KEY_MODE_1: break; /* TODO: 模式1循环任务。 */
        case KEY_MODE_2: break; /* TODO: 模式2循环任务。 */
        case KEY_MODE_3: break; /* TODO: 模式3循环任务。 */
        case KEY_MODE_4: break; /* TODO: 模式4循环任务。 */
        default: break;
    }
}

Key_Mode Key_GetMode(void)
{
    return s_mode;
}

uint8_t Key_TakePresses(void)
{
    uint32_t irq_state = __get_PRIMASK();
    uint8_t presses;
    __disable_irq();
    presses = s_press_events;
    s_press_events = 0U;
    if (irq_state == 0U) __enable_irq();
    return presses;
}
