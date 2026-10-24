#include <stdint.h>
#include "motor.h"
#include "delay.h"
#include "pid_motor.h"
#include "uart.h"
#include "key.h"
//#include "grayscale_sensor.h"
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    // 清除上电时可能残留的挂起中断，防止误触发
    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    // 使能 GPIO 中断。进入这一步后，只要电机转动，就会自动进入中断
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);

    NVIC_ClearPendingIRQ(pid_timer_INST_INT_IRQN);
    NVIC_EnableIRQ(pid_timer_INST_INT_IRQN);
    DL_TimerG_startCounter(pid_timer_INST);
    //keymode last_mode=key_idle;
    while (1) 
    {
        // keyscan();
        // keymode current_mode=key_getmode();
        // if(current_mode!=last_mode){
        //     condition_rst(state_line);//不同模式清0
        //     last_mode=current_mode;
        // }
        // switch(current_mode){
        //     case key1_putong:

        //         break;
            
        //     case key2_teshu:

        //         break;

        //     case key3_baidong:

        //         break;
            
        //     case key_idle:
        //         set_target(0,0);
        //         break;
        //     default:
        //         set_target(0,0);
        //         break;
        // }
        motor_rlencoder_cnts();
        delay_ms(10);
    }
    return 0;
}
    