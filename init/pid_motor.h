#ifndef PID_MOTOR_H_
#define PID_MOTOR_H_

#include <stdint.h>

typedef struct {
    int32_t integral;
    int32_t previous_error;
    int32_t previous_target;
} PIDMotor_Controller;

extern volatile int32_t right_encoder_count;
extern volatile int right_error_count;
extern volatile int32_t left_encoder_count;
extern volatile int left_error_count;
extern volatile int right_count_10ms;
extern volatile int left_count_10ms;
extern volatile int right_error;
extern volatile int left_error;

extern volatile int right_output;
extern volatile int left_output;

extern volatile int init_rightpwm;
extern volatile int init_leftpwm;


extern PIDMotor_Controller left_pid;
extern PIDMotor_Controller right_pid;

#define pid_motor_gain_scale             (100)
#define motor_kp             (250)
#define motor_ki             (3)
#define motor_kd             (0)
#define integral_limit         (500)
#define max_count_10ms  (58)

#define encoder_left_dir        (-1)
#define encoder_right_dir       (1)


extern int32_t Clamp(int32_t value, int32_t min, int32_t max);
int32_t Absolute(int32_t value);
int Sign(int32_t value);
int32_t PercentToCounts(int32_t percent);
int pid_motor(PIDMotor_Controller *controller,int target_percent, int target_counts, int measured_counts);
void pidmotor_rst(PIDMotor_Controller *controller);

#endif 
