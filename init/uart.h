#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "grayscale_sensor.h"

void UART_SendString(const char *string);
void motor_encoder_condition(void);
void motor_rlencoder_cnts(const grays *sample, keymode mode);
void motor_encoder_cnt(void);

#endif 
