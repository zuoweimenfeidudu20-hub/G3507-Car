#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

void RightMotor_SetSpeed(uint32_t percent);
void LeftMotor_SetSpeed(uint32_t percent);
void RightMotor_Stop(void);
void RightMotor_Forward(uint32_t speed);
void RightMotor_Reverse(uint32_t speed);
void LeftMotor_Stop(void);
void LeftMotor_Forward(uint32_t speed);
void LeftMotor_Reverse(uint32_t speed);
void AllMotor_Stop(void);
uint32_t Motor_Clamp(int32_t percent);

#endif
