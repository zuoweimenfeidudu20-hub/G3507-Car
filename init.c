#include "ti_msp_dl_config.h"
#include "motor.h"
#include "pid_motor.h"
#include "uart.h"
#include "key.h"
#include "grayscale_sensor.h"

#include <stdint.h>

grays sample;
keymode last_mode=key4_idle;
int main(void)
{
    SYSCFG_DL_init();
    set_target(0, 0);
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


    while (1)
    {
        keymode current_mode=key_getmode();
        if(current_mode!=last_mode){
            track_normal_reset();
            condition_rst2(state2_line);
            set_target(0, 0);
            last_mode = current_mode;
        }

        if (gray_sample(&sample)) {
            trackcontroll(current_mode, &sample);
            motor_encoder_condition();
        }
    }
}
