#include "ti_msp_dl_config.h"
#include "pid_motor.h"
#include "uart.h"
#include <stdio.h>
#include <stdint.h>

void UART_SendString(const char *string)
{
    while (*string != '\0') {
        DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t)*string);
        string++;
    }
}

void motor_encoder_condition(void)
{
    // 定义一个能容纳 64 个字符的临时数组
    char tx_buffer[192]; 
    // %d 代表十进制有符号整数
    snprintf(tx_buffer, sizeof(tx_buffer), 
            "%d,%d,%d,%d,%d,%d,%d,%d\r\n", 
            init_rightpwm, right_output, left_output,
            right_count_10ms, -left_count_10ms,
            PercentToCounts(init_rightpwm), 
            left_pid.integral,right_pid.integral);
    UART_SendString(tx_buffer);
}

