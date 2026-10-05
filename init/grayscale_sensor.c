#include "ti_msp_dl_config.h"
#include "grayscale_sensor.h"
#include "pid_motor.h"
#include "motor.h"

#define base_pwm 35
#define track_scale 100

//先调ki,后调kp,kd
static track_pid control={
    .kp=0,
    .ki=0,
    .kd=0,

    .integral=0,
    .previous_error=0,

    .track_int_limit=500,//积分限幅
    .output_limit=5//输出限幅
};


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

//状态机，地图状态
typedef enum
{
    pattern_line,//普通状况
    pattern_allblack,//全黑
    pattern_rightbranch,//右边路线存在分支
    pattern_r90//直角弯
}trackpattern;
//小车任务，该怎么走
typedef enum
{
    state_common,//pid正常循迹
    state_enterbranch,//进入分支
    state_turn_right,
    state_turn_left,
    state_right90,
    state_stop,
    //track_obstacle
}trackstate;

//记录路径状态，函数只能返回3种状态（实际不能返回正常的电平数据？）
trackpattern track_analyze(int values[8],int *error)
{
    int weighted_sum=0;
    int right_count=0;
    int left_count=0;
    int cnt=0;//黑

    for(int i=0;i<8;i++){
        if(values[i]==1){
            weighted_sum+=weights[i];
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
    // if(cnt==0){
    //     *error=
    // }
   //识别多黑，进入岔路状态
    if (cnt==8)
    {
        return track_stop;//
    }
    if(cnt>=6){
        *error=0;//??
        return ;
    }
    if (right_count>=3&&left_count==0)
    {
        *error=3500;//?
        return track_right90;
    }
    //误差,普通循迹
    *error=weighted_sum/cnt;
    return track_common;
}

void track_controll(void)
{
    int sensors[8]={0};
    int error=0;
    int correction=0;
    trackstate state;

    sensor_read(sensors);
    
    state=track_analyze(sensors,&error);
   
    switch(state)
    {
        case track_ok:
            correction=line_pid(&control,error);
            set_target(base_pwm,correction);
            break;

        case track_cross:
            //区分按键状态1，2
            //1


            //2
            set_target(base_pwm,0);
            break;

        // case track_right90:
        //     //结合陀螺仪
        //     set_target(base_pwm,)

        //     break;

        case track_stop:
            allstop();
        
    }
}

//两次状态确认进入某状态
bool track_confirm()




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


