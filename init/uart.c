#include "ti_msp_dl_config.h"
#include "pid_motor.h"
#include "uart.h"
#include <stdio.h>
#include <stdint.h>

#define UART_TX_WAIT_LIMIT 4096U

volatile uint32_t uart_tx_timeout_count = 0;

void UART_SendString(const char *string)
{
    while (*string != '\0') {
        uint32_t tries = 0;
        while (!DL_UART_Main_transmitDataCheck(UART_0_INST, (uint8_t)*string)) {
            if (++tries >= UART_TX_WAIT_LIMIT) {
                uart_tx_timeout_count++;
                return;
            }
        }
        string++;
    }
}

void motor_encoder_condition(void)
{
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

void motor_rlencoder_cnts(void)
{
    char tx_buffer[200];
    snprintf(tx_buffer,sizeof(tx_buffer),
            "%d,%d\r\n",
            right_encoder_count,left_encoder_count);
    UART_SendString(tx_buffer);
}
