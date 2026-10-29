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

void motor_encoder_cnt(void)
{
    char tx_buffer[192]; 
    // %d 代表十进制有符号整数
    snprintf(tx_buffer, sizeof(tx_buffer), 
            "%d,%d\r\n", 
            right_encoder_count,left_encoder_count);
    UART_SendString(tx_buffer);
}

static const char *pattern_name(trackpattern pattern)
{
    switch(pattern){
        case pattern_line: return "pattern_line";
        case pattern_allblack: return "pattern_allblack";
        case pattern_rightbranch: return "pattern_rightbranch";
        case pattern_leftbranch: return "pattern_leftbranch";
        case pattern_r90: return "pattern_r90";
        case pattern_lost: return "pattern_lost";
        default: return "pattern_unknown";
    }
}

static const char *key_mode_name(keymode mode)
{
    switch(mode){
        case key1_putong: return "key1_putong";
        case key2_teshu: return "key2_teshu";
        case key3_baidong: return "key3_baidong";
        case key4_idle: return "key4_idle";
        default: return "key_unknown";
    }
}

static const char *current_state_name(keymode mode)
{
    if(mode==key1_putong){
        switch(current_state1){
            case state1_line: return "state1_line";
            case state1_c_lockstraight1: return "state1_c_lockstraight1";
            case state1_c_lockstraight2: return "state1_c_lockstraight2";
            case state1_stop: return "state1_stop";
            case state1_fault: return "state1_fault";
            default: return "state1_unknown";
        }
    }
    if(mode==key2_teshu){
        switch(current_state2){
            case state2_line: return "state2_line";
            case state2_c_lockright: return "state2_c_lockright";
            case state2_stair1: return "state2_stair1";
            case state2_teshu_lockleft1: return "state2_teshu_lockleft1";
            case state2_branch_lockleft: return "state2_branch_lockleft";
            case state2_stair2: return "state2_stair2";
            case state2_teshu_lockright2: return "state2_teshu_lockright2";
            case state2_stop: return "state2_stop";
            case state2_fault: return "state2_fault";
            default: return "state2_unknown";
        }
    }
    return "inactive";
}

void motor_rlencoder_cnts(const grays *sample, keymode mode)
{
    if(sample==NULL){
        return;
    }

    char gray_bits[gray_sensor+1];
    for(int i=0;i<gray_sensor;i++){
        gray_bits[i]=sample->values[i] ? '1' : '0';
    }
    gray_bits[gray_sensor]='\0';

    char tx_buffer[192];
    snprintf(tx_buffer,sizeof(tx_buffer),
            "gray=%s,pattern=%s,key=%s,state=%s,pwmL=%d,pwmR=%d\r\n",
            gray_bits,pattern_name(sample->pattern),key_mode_name(mode),current_state_name(mode),
            left_output,right_output);
    UART_SendString(tx_buffer);
}
