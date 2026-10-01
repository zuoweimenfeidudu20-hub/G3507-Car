#include "ti_msp_dl_config.h"
#include "motor.h"

#define PWM_PERIOD 1600U

void RightMotor_SetSpeed(uint32_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    } 

    uint32_t compare = PWM_PERIOD -
                       ((PWM_PERIOD * percent) / 100U);

    DL_TimerA_setCaptureCompareValue(PWM_motor_INST,compare,GPIO_PWM_motor_C0_IDX);
}

void LeftMotor_SetSpeed(uint32_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }

    uint32_t compare = PWM_PERIOD -
                       ((PWM_PERIOD * percent) / 100U);

    DL_TimerA_setCaptureCompareValue(PWM_motor_INST,compare,GPIO_PWM_motor_C1_IDX);
}


void RightMotor_Stop(void)
{
    DL_GPIO_clearPins(MOTOR_DIR1_AIN1_PORT, MOTOR_DIR1_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR1_AIN2_PORT, MOTOR_DIR1_AIN2_PIN);
    RightMotor_SetSpeed(0);
}


void RightMotor_Forward(uint32_t speed)
{
    DL_GPIO_setPins(MOTOR_DIR1_AIN1_PORT, MOTOR_DIR1_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR1_AIN2_PORT, MOTOR_DIR1_AIN2_PIN);
    RightMotor_SetSpeed(speed);
}

void RightMotor_Reverse(uint32_t speed)
{
    DL_GPIO_clearPins(MOTOR_DIR1_AIN1_PORT, MOTOR_DIR1_AIN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR1_AIN2_PORT, MOTOR_DIR1_AIN2_PIN);
    RightMotor_SetSpeed(speed);
}


void LeftMotor_Stop(void)
{
    DL_GPIO_clearPins(MOTOR_DIR2_BIN1_PORT, MOTOR_DIR2_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR2_BIN2_PORT, MOTOR_DIR2_BIN2_PIN);
    LeftMotor_SetSpeed(0);
}

void LeftMotor_Forward(uint32_t speed)
{
    DL_GPIO_setPins(MOTOR_DIR2_BIN1_PORT, MOTOR_DIR2_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR2_BIN2_PORT, MOTOR_DIR2_BIN2_PIN);

    LeftMotor_SetSpeed(speed);
}

void LeftMotor_Reverse(uint32_t speed)
{
    DL_GPIO_clearPins(MOTOR_DIR2_BIN1_PORT, MOTOR_DIR2_BIN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR2_BIN2_PORT, MOTOR_DIR2_BIN2_PIN);
    LeftMotor_SetSpeed(speed);
}

void AllMotor_Stop(void)
{
    RightMotor_Stop();
    LeftMotor_Stop();
}

uint32_t Motor_Clamp(int32_t percent)
{
    if ((percent > 100) || (percent < -100)) {
        return 100;
    }
    if (percent < 0) {
        percent = -percent;
    }
    return (uint32_t) percent;
}
