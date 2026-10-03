#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

void rightmotor_pwm(uint32_t percent);
void leftmotor_pwm(uint32_t percent);
void rightmotor_stop(void);
void rightmotor_forward(uint32_t speed);
void rightmotor_reverse(uint32_t speed);
void leftmotor_stop(void);
void leftmotor_forward(uint32_t speed);
void leftmotor_reverse(uint32_t speed);
void allstop(void);
int32_t motor_clamp(int32_t percent);

void rightmotor_output(int32_t output);
void leftmotor_output(int32_t output);
#endif
