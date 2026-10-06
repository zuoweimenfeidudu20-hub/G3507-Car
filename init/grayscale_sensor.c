#include "ti_msp_dl_config.h"
#include "grayscale_sensor.h"
#include "pid_motor.h"
#include "motor.h"

//mission run_mode=normal;


typedef struct {
    GPIO_Regs *port;
    uint32_t pin;
} Grayscale_Pin;


//左到右
static const Grayscale_Pin pins[gray_sensor] = {
    {Grays_GRAY1_PORT, Grays_GRAY1_PIN},
    {Grays_GRAY2_PORT, Grays_GRAY2_PIN},
    {Grays_GRAY3_PORT, Grays_GRAY3_PIN},
    {Grays_GRAY4_PORT, Grays_GRAY4_PIN},
    {Grays_GRAY5_PORT, Grays_GRAY5_PIN},
    {Grays_GRAY6_PORT, Grays_GRAY6_PIN},
    {Grays_GRAY7_PORT, Grays_GRAY7_PIN},
    {Grays_GRAY8_PORT, Grays_GRAY8_PIN}
};

static const int weights[8]={-3500,-2500,-1500,-500,500,1500,2500,3500};

void sensor_read(int values[gray_sensor])
{
    uint8_t current_level;
    for (int i=0;i<gray_sensor;i++) {
        current_level=(DL_GPIO_readPins(pins[i].port, pins[i].pin) != 0) ? 1 : 0;
        if(current_level==black_level)
        {
            values[i] = 1;
        }
        else
        {
            values[i] = 0;
        }
    }
}

//计算识别到黑线传感器数量  ??逻辑??
int track_count(const int values[8])
{
    int count=0;
    for(int i=0;i<8;i++){
        if(values[i]==1){
            count+=values[i];
        }
    }
    return count;
}

//模式1，2
typedef enum
{
    normal,
    abnormal,
    baidong
}mission;

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
//小车任务，该怎么走   1,2模式区分
typedef enum
{
    state_line,//楼梯状态前
    state_c_lockstraight,//模式1：cross路口直行
    state_c_lockright,//2:锁定右转
    state_stair1,//1楼梯内部普通巡线
    state_teshu_lockleft1,//中间识别到白色，锁定左转直到最右边传感器识别到黑线
    state_branch_lockleft,//识别到左边黑色延申，锁定左转直到右边传感器识别到黑线
    state_stair2,//2普通寻
    state_teshu_lockright2,//中间识别到白色，锁定右转
    state_f,//出楼梯---循迹结束
    state_stop,
    state_fault
}trackstate;

//记录路径状态，函数只能返回trackpattern的状态
trackpattern track_analyze(int values[gray_sensor],int *error)
{
    int weight_sum=0;
    int right_count=0;
    int left_count=0;
    int cnt=0;//黑

    *error=0;
    for(int i=0;i<gray_sensor;i++){
        if(values[i]==1){
            weight_sum+=weights[i];
            cnt++;
            if (i>=5)
            {
                right_count++;
            }
            if (i<=2)
            {
                left_count++;
            }
        }
    }

    //丢先=线追上一次线路
    if(cnt==0){
        return pattern_lost;
    }
    *error=weight_sum/cnt;
   //识别多黑，进入岔路状态
    if (cnt==gray_sensor)
    {
        return pattern_allblack;
    }
    bool center_gray=values[3]||values[4];
    //特殊路况
    if (center_gray&&right_count>=1&&left_count==0)
    {
        return pattern_rightbranch;
    }
    if(center_gray&&left_count>=1&&right_count==0)
    {
        return pattern_leftbranch;
    }
    //直角弯
    if(center_gray&&right_count>=3&&left_count==0)
    {
        return pattern_r90;
    }
    return pattern_line;
}

//??
trackpattern control_state;
trackstate current_state=state_line;
int state_ticks=0;
int current_cnt=0;
bool event_condition=false;

void track_controll(void)
{
    int sensors[gray_sensor]={0};
    int error=0;
    int correction=0;

    trackpattern pattern;

    sensor_read(sensors);
    pattern=track_analyze(sensors,&error);//分析trackpattern状态
   
    state_ticks++;
    bool maxleft=sensors[0]||sensors[1];//最左边两个任一
    bool maxright=sensors[6]||sensors[7];//同
    //嵌套状态，外层：小车当前执行状态
    switch(control_state)
    {
        case current_state:
            switch(pattern)
            {
                case pattern_line:
                correction=line_pid(&control,error);
                set_target(base_pwm,correction);
                break;

                case pattern_lost:

            }
    }
}

//两次状态确认进入某状态,滤波,require    只返回1/0
bool track_confirm(bool condition,int require)
{
    if(condition){
        if(current_cnt<require){
            current_cnt++;
        }
    }
    else{
        current_cnt=0;
    }
    return current_cnt>=require;
}

//中间传感器黑变白
bool center_bianhua(int sensors[gray_sensor])
{
    
}

//切换状态清0
void condition_rst(trackstate next)
{
    current_state=next;
    state_ticks=0;
    current_cnt=0;
    event_condition=false;

    trackpid_rst(&control,0);
}

//先调ki,后调kp,kd
track_pid control={
    .kp=0,
    .ki=0,
    .kd=0,

    .integral=0,
    .previous_error=0,

    .track_int_limit=500,//积分限幅
    .output_limit=5//输出限幅
};
//输出correction纠正力度大小
int line_pid(track_pid *pid,int error)
{
    int err=error;
    //累计误差
    pid->integral+=error;
    //积分限幅
    pid->integral=Clamp(pid->integral,-(pid->track_int_limit),pid->track_int_limit);
    //误差变化情况
    int derivative=error-pid->previous_error;

    int32_t output;
    output=(int32_t)(pid->kp*(err)+pid->ki*(pid->integral)+pid->kd*(derivative))/track_scale;
    output=Clamp(output,-(pid->output_limit),pid->output_limit);
    pid->previous_error=error;
    return output;
}

//pid_rst
void trackpid_rst(track_pid *pid,int error)
{
    pid->integral=0;
    pid->previous_error=error;
}

//目标pwm设置,普通前进状态
void set_target(int pwm,int correction)
{
    int left_target=pwm+correction;
    int right_target=pwm-correction;

    left_target=Clamp(left_target,0,100);
    right_target=Clamp(right_target,0,100);

    init_rightpwm=right_target;
    init_leftpwm=left_target;
}

//直角，避障90°转弯
void set_target90r()


