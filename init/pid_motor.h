#ifndef PID_MOTOR_H_
#define PID_MOTOR_H_

#include <stdint.h>

extern volatile int right_encoder_count;
extern volatile int right_error_count;


#define PID_MOTOR_GAIN_SCALE             (100)
#define motor_kp             (250)
#define motor_ki             (3)
#define motor_kd             (0)
#define pid_motor_integral_limit         (500)
#define max_count_10ms  (58)

#define encoder_left_dir        (-1)
#define encoder_right_dir       (1)



#endif /* PID_MOTOR_H_ */
