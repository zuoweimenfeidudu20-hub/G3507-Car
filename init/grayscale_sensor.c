#include "ti_msp_dl_config.h"
#include "grayscale_sensor.h"
#include "pid_motor.h"
#include "motor.h"

typedef struct {
    GPIO_Regs *port;
    uint32_t pin;
} Grayscale_Pin;

//左到右
static const Grayscale_Pin pins[grayscale_sensor] = {
    {Grays_GRAY1_PORT, Grays_GRAY1_PIN},
    {Grays_GRAY2_PORT, Grays_GRAY2_PIN},
    {Grays_GRAY3_PORT, Grays_GRAY3_PIN},
    {Grays_GRAY4_PORT, Grays_GRAY4_PIN},
    {Grays_GRAY5_PORT, Grays_GRAY5_PIN},
    {Grays_GRAY6_PORT, Grays_GRAY6_PIN},
    {Grays_GRAY7_PORT, Grays_GRAY7_PIN},
    {Grays_GRAY8_PORT, Grays_GRAY8_PIN}
};

void sensor_read(int values[GRAYSCALE_SENSOR_COUNT])
{
    int i;

    for (i = 0; i < GRAYSCALE_SENSOR_COUNT; i++) {
        if(DL_GPIO_readPins(pins[i].port, pins[i].pin)==BLACK_LEVEL)
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

static const values[8]={-3500,-2500,-1500,-500,500,1500,2500,3500};

//状态机
typedef enum
{
    track_ok,
    track_cross,
    track_right90,
    track_stop
}trackstate;

//记录位置误差
trackstate track_analyze(int values[8],int *error)
{
    int weighted_sum=0;
    int right_count=0;
    int left_count=0;
    int cnt=0;//黑

    //
    for(int i=0;i<8;i++){
        if(values[i]==1){
            weighted_sum+=values[i];
            cnt++;
            if (i>=6)
            {
                right_count++;
            }
            if (i<=3)
            {
                left_count++;
            }
        }
    }

   //识别多黑，进入岔路状态
    if(cnt>=6){
        *error=0;//??
        return track_cross;
    }
    if (cnt==8)
    {
        return track_stop;
    }
    if (right_count>=3&&left_count==0)
    {
        *error=3500;//?
        return track_right90;
    }
    //误差,普通循迹
    *error=weighted_sum/cnt;
    return track_ok;
}

//pid输出correction
int line_pid(track_pid *pid,int error)
{
    int err=error;
    //累计误差
    pid->integral+=error;
    //积分限幅
    pid->integral=Clamp(pid->integral,-integral_limit,integral_limit);
    //误差变化情况
    int derivative=error-pid->previous_error;

    int output;
    output=pid->kp*err+pid->ki*pid->integral+pid->kd*derivative;
    output=Clamp(output,-output_limit,output_limit);
    return output;
}

//目标pwm设置
void set_target(int base_pwm,int correction)
{
    int left_target=base_pwm+correction;
    int right_target=base_pwm-correction;

    left_target=Clamp(left_target,0,100);
    right_target=Clamp(right_target,0,100);

    init_rightpwm=right_target;
    init_leftpwm=left_target;
}

void track_controll(void)
{
    int sensors[8];
    int error;
    int correction;
    trackstate state;
    sensor_read(sensors);
    state=



    switch()
    {



    } 
    
}