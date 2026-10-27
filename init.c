#include "ti_msp_dl_config.h"
#include "motor.h"
#include "delay.h"
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
    //
    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_ClearPendingIRQ(pid_timer_INST_INT_IRQN);
    NVIC_EnableIRQ(pid_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(pid_timer_INST);
    //
    DL_TimerG_clearInterruptStatus(track_timer_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(track_timer_INST_INT_IRQN);
    NVIC_EnableIRQ(track_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(track_timer_INST);

    while (1) 
    {
        keyscan();
        keymode current_mode=key_getmode();
        if(current_mode!=last_mode){
            condition_rst1(state1_line);//不同模式清0
            condition_rst2(state2_line);
            last_mode=current_mode;
        }
        if(gray_sample(&sample)){
        trackcontroll(current_mode, &sample);
    }
        motor_rlencoder_cnts();
    }
    motor_rlencoder_cnts();
    delay_ms(100);
}

    