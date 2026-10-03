#include <stdint.h>
#include "motor.h"
#include "delay.h"
#include "pid_motor.h"
#include "uart.h"
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
    
    while (1) 
    {
        motor_encoder_condition();
        delay_ms(100);
    }
    return 0;
}
    