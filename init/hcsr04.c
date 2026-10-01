#include "hcsr04.h"
#include "ti_msp_dl_config.h"

/* 32 MHz / 8 / 8 = 500 kHz: 29 ticks per cm (58 us per cm). */
#if hcsr04_timer_INST_LOAD_VALUE != 49999U
#error "HCSR04 requires TIMG6 at 500 kHz with a 100 ms period"
#endif

enum { IDLE, WAIT_RISE, WAIT_FALL };
static volatile uint32_t s_state;
static volatile uint32_t s_rise_count;
static volatile uint32_t s_echo_ticks;
static volatile uint32_t s_enabled;

static void HCSR04_Trigger(void)
{
    s_state = IDLE;
    DL_GPIO_clearInterruptStatus(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN);
    /* A stuck/late echo must finish before another measurement can start. */
    if (DL_GPIO_readPins(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) != 0U) {
        s_echo_ticks = 0U;
        return;
    }
    s_state = WAIT_RISE;
    DL_GPIO_setPins(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
    delay_cycles(CPUCLK_FREQ / 100000U); /* 10 us trigger; no echo polling. */
    DL_GPIO_clearPins(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
}

void HCSR04_Init(void)
{
    uint32_t saved_mask = __get_PRIMASK();
    __disable_irq();
    DL_TimerG_stopCounter(hcsr04_timer_INST);
    DL_GPIO_clearPins(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
    s_state = IDLE;
    s_echo_ticks = 0U;
    s_enabled = 1U;
    DL_TimerG_setTimerCount(hcsr04_timer_INST, hcsr04_timer_INST_LOAD_VALUE);
    DL_TimerG_clearInterruptStatus(hcsr04_timer_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(hcsr04_timer_INST_INT_IRQN);
    /* Same priority as GPIO: timeout and edge processing cannot preempt each other. */
    NVIC_SetPriority(hcsr04_timer_INST_INT_IRQN, 0U);
    NVIC_SetPriority(GPIO_MULTIPLE_GPIOA_INT_IRQN, 0U);
    DL_GPIO_enableInterrupt(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(hcsr04_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(hcsr04_timer_INST);
    HCSR04_Trigger();
    if (saved_mask == 0U) __enable_irq();
}

float HCSR04_GetDistance(void)
{
    uint32_t ticks = s_echo_ticks;
    return (ticks == 0U) ? -1.0f : (float) ticks / 29.0f;
}

/* Internal hook called first by the shared GPIO GROUP1 interrupt handler. */
void HCSR04_HandleEchoInterrupt(void)
{
    uint32_t count;
    uint32_t high;
    if (DL_GPIO_getEnabledInterruptStatus(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == 0U)
        return;
    count = DL_TimerG_getTimerCount(hcsr04_timer_INST);
    high = DL_GPIO_readPins(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN);
    DL_GPIO_clearInterruptStatus(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN);
    /* Reject edges from a period whose timeout ISR has not yet executed. */
    if (s_enabled == 0U || DL_TimerG_getRawInterruptStatus(
            hcsr04_timer_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT) != 0U)
        return;
    if (high != 0U && s_state == WAIT_RISE) {
        s_rise_count = count;
        s_state = WAIT_FALL;
    } else if (high == 0U && s_state == WAIT_FALL) {
        uint32_t ticks = (s_rise_count >= count) ? s_rise_count - count : 0U;
        /* HC-SR04 nominal measurement range: 2..400 cm. */
        s_echo_ticks = (ticks >= 58U && ticks <= 11600U) ? ticks : 0U;
        s_state = IDLE;
    }
}

void hcsr04_timer_INST_IRQHandler(void)
{
    if (DL_TimerG_getPendingInterrupt(hcsr04_timer_INST) == DL_TIMERG_IIDX_ZERO) {
        if (s_state != IDLE) s_echo_ticks = 0U;
        HCSR04_Trigger();
    }
}
