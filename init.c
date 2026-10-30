#include "ti_msp_dl_config.h"
#include "motor.h"
#include "pid_motor.h"
#include "uart.h"
#include "key.h"
#include "grayscale_sensor.h"

#include <stdint.h>
#include <stdio.h>

grays sample;
keymode last_mode = key4_idle;

static const char *reset_cause_name(DL_SYSCTL_RESET_CAUSE cause)
{
    switch (cause)
    {
        case DL_SYSCTL_RESET_CAUSE_POR_HW_FAILURE: return "por_hw";
        case DL_SYSCTL_RESET_CAUSE_BOR_SUPPLY_FAILURE: return "bor_supply";
        case DL_SYSCTL_RESET_CAUSE_POR_EXTERNAL_NRST:
        case DL_SYSCTL_RESET_CAUSE_BOOTRST_EXTERNAL_NRST: return "nrst_pin";
        case DL_SYSCTL_RESET_CAUSE_SYSRST_CPU_LOCKUP_VIOLATION: return "cpu_lockup";
        case DL_SYSCTL_RESET_CAUSE_NO_RESET: return "no_reset";
        default: return "other";
    }
}

int main(void)
{
    DL_SYSCTL_RESET_CAUSE reset_cause = DL_SYSCTL_getResetCause();
    SYSCFG_DL_init();
    set_target(0, 0);

    char diagnostic[96];
    snprintf(diagnostic, sizeof(diagnostic),
             "event=boot,reset=0x%02x,reason=%s\r\n",
             (unsigned int)reset_cause, reset_cause_name(reset_cause));
    UART_SendString(diagnostic);

    DL_TimerG_clearInterruptStatus(key_timer_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(key_timer_INST_INT_IRQN);
    NVIC_EnableIRQ(key_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(key_timer_INST);

    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_ClearPendingIRQ(pid_timer_INST_INT_IRQN);
    NVIC_EnableIRQ(pid_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(pid_timer_INST);


    DL_TimerG_clearInterruptStatus(track_timer_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(track_timer_INST_INT_IRQN);
    NVIC_EnableIRQ(track_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(track_timer_INST);

    uint32_t last_key4_confirm_count = key_get_key4_confirm_count();
    unsigned int telemetry_samples = 0;
    while (1)
    {
        keymode current_mode = key_getmode();
        if (current_mode != last_mode)
        {
            track_normal_rst();
            track_abnormal_rst();
            set_target(0, 0);
            last_mode = current_mode;
        }

        uint32_t key4_confirm_count = key_get_key4_confirm_count();
        if (key4_confirm_count != last_key4_confirm_count)
        {
            snprintf(diagnostic, sizeof(diagnostic),
                     "event=key4_confirmed,count=%lu\r\n",
                     (unsigned long)key4_confirm_count);
            UART_SendString(diagnostic);
            last_key4_confirm_count = key4_confirm_count;
        }

        if (gray_sample(&sample))
        {
            trackcontroll(current_mode, &sample);

            if (key_getmode() != current_mode)
            {
                set_target(0, 0);
            }

            if (++telemetry_samples >= 5)
            {
                telemetry_samples = 0;
                motor_rlencoder_cnts(&sample,current_mode);
            }
        }
       // set_target(15,0);
    }
}
