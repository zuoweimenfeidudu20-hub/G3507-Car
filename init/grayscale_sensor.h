#ifndef GRAYSCALE_SENSOR_H_
#define GRAYSCALE_SENSOR_H_

#include <stdint.h>

#define gray_sensor 8
#define black_level 1

#define integral_limit 200//积分限幅
#define output_limit 
#define pid_scale 
#define kp 
#define ki
#define kd

typedef struct
{
    int kp;
    int ki;
    int kd;

    int integral;
    int previous_error;

    int int_limit;
    int output_limit;
}track_pid;
#endif 

