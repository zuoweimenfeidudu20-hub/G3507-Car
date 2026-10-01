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
    NVIC_ClearPendingIRQ(GPIO_ENCODER_INT_IRQN);
    
    // 使能 GPIO 中断。进入这一步后，只要电机转动，就会自动进入中断
    NVIC_EnableIRQ(GPIO_ENCODER_INT_IRQN);

    // 主循环：负责每隔一段时间打印当前的 cnt 值
    while (1) 
    {
        
        delay_ms(200); 
    }
    return 0;
}
    