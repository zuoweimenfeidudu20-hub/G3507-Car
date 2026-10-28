#ifndef GRAYSCALE_SENSOR_H_
#define GRAYSCALE_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "key.h"

#define gray_sensor 8
#define black_level 1


#define base_pwm 35
#define track_scale 100


//状态机，地图状态
typedef enum
{
    pattern_line,//普通状况
    pattern_allblack,//全黑
    pattern_rightbranch,//右边路线存在分支
    pattern_leftbranch,//
    pattern_r90,//直角弯
    pattern_lost
}trackpattern;

typedef struct
{
    trackpattern pattern;
    int error;
}grays;

//小车任务，该怎么走   1,2模式区分
typedef enum
{
    state1_line,//楼梯状态前/普通循迹
    state1_c_lockstraight1,//模式1：cross路口直行
    state1_c_lockstraight2,//1:第2段路程cross直行
    state1_stop,
    state1_fault,
}trackstate1;

typedef enum
{
    state2_line,
    state2_c_lockright,//2:锁定右转
    state2_stair1,//1楼梯内部普通巡线
    state2_teshu_lockleft1,//中间识别到白色，锁定左转直到最右边传感器识别到黑线
    state2_branch_lockleft,//识别到左边黑色延申，锁定左转直到右边传感器识别到黑线
    state2_stair2,//2楼梯线路过程中的普通循迹
    state2_teshu_lockright2,//中间识别到白色，锁定右转
    state2_stop,
    state2_fault,
}trackstate2;

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
trackpattern track_analyze(int values[gray_sensor],int *error);
int line_pid(track_pid *pid,int error);
void set_target(int pwm,int correction);
extern void condition_rst1(trackstate1 next);
extern void condition_rst2(trackstate2 next);
void trackpid_rst(track_pid *pid,int error);
int track_count(int values[8]);
bool track_confirm(bool condition,int require);
bool center_bianhua(int sensors[gray_sensor]);

void trackcontroll(keymode mode,const grays *sample);
void track_run_normal(trackpattern pattern,int error);
void track_normal_reset(void);

bool gray_sample(grays *sample);
#endif 

