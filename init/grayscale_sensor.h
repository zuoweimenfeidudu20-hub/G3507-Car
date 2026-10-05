#ifndef GRAYSCALE_SENSOR_H_
#define GRAYSCALE_SENSOR_H_

#include <stdint.h>

#define gray_sensor 8
#define black_level 1


#define base_pwm 35

//结构体位置有要求吗
typedef struct
{
    int kp;
    int ki;
    int kd;

    int integral;
    int previous_error;

    int track_int_limit;//积分限幅
    int output_limit;
}track_pid;


void sensor_read(int values[gray_sensor]);
int track_count(const int values[8]);
trackstate track_analyze(int values[8],int *error);
void track_controll(void);
int line_pid(track_pid *pid,int error);
void set_target(int pwm,int correction);

#endif 

