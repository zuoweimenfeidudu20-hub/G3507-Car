#include "ti_msp_dl_config.h"
#include "motor.h"
#include "pid_motor.h"

#define PWM_PERIOD 1600U

void rightmotor_pwm(uint32_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }

    uint32_t compare = PWM_PERIOD -
                       ((PWM_PERIOD * percent) / 100U);

    DL_TimerA_setCaptureCompareValue(PWM_motor_INST,compare,GPIO_PWM_motor_C0_IDX);
}

void leftmotor_pwm(uint32_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }

    uint32_t compare = PWM_PERIOD -
                       ((PWM_PERIOD * percent) / 100U);

    DL_TimerA_setCaptureCompareValue(PWM_motor_INST,compare,GPIO_PWM_motor_C1_IDX);
}


void rightmotor_stop(void)
{
    DL_GPIO_clearPins(MOTOR_DIR1_AIN1_PORT, MOTOR_DIR1_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR1_AIN2_PORT, MOTOR_DIR1_AIN2_PIN);
    rightmotor_pwm(0);
}


void rightmotor_forward(uint32_t speed)
{
    DL_GPIO_setPins(MOTOR_DIR1_AIN1_PORT, MOTOR_DIR1_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR1_AIN2_PORT, MOTOR_DIR1_AIN2_PIN);
    rightmotor_pwm(speed);
}

void rightmotor_reverse(uint32_t speed)
{
    DL_GPIO_clearPins(MOTOR_DIR1_AIN1_PORT, MOTOR_DIR1_AIN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR1_AIN2_PORT, MOTOR_DIR1_AIN2_PIN);
    rightmotor_pwm(speed);
}


void leftmotor_stop(void)
{
    DL_GPIO_clearPins(MOTOR_DIR2_BIN1_PORT, MOTOR_DIR2_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR2_BIN2_PORT, MOTOR_DIR2_BIN2_PIN);
    leftmotor_pwm(0);
}

void leftmotor_forward(uint32_t speed)
{
    DL_GPIO_setPins(MOTOR_DIR2_BIN1_PORT, MOTOR_DIR2_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR2_BIN2_PORT, MOTOR_DIR2_BIN2_PIN);

    leftmotor_pwm(speed);
}

void leftmotor_reverse(uint32_t speed)
{
    DL_GPIO_clearPins(MOTOR_DIR2_BIN1_PORT, MOTOR_DIR2_BIN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR2_BIN2_PORT, MOTOR_DIR2_BIN2_PIN);
    leftmotor_pwm(speed);
}

void allstop(void)
{
    rightmotor_stop();
    leftmotor_stop();
}

int32_t motor_clamp(int32_t percent)
{
    if (percent > 100) {
        return 100;
    }
    if (percent < -100) {
        return -100;
    }
    return percent;
}

void rightmotor_output(int32_t output)
{
    output=motor_clamp(output);
    if(output>0){
        rightmotor_forward((uint32_t)output);
    }else if(output<0){
        rightmotor_reverse((uint32_t)(-output));
    }else{
        rightmotor_stop();
    }
}

void leftmotor_output(int32_t output)
{
    output=motor_clamp(output);
    if(output>0){
        leftmotor_forward((uint32_t)output);
    }else if(output<0){
        leftmotor_reverse((uint32_t)(-output));
    }else{
        leftmotor_stop();
    }
}
