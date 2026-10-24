#include "ti_msp_dl_config.h"
#include "grayscale_sensor.h"
#include "pid_motor.h"
#include "motor.h"

//mission run_mode=normal;

int state_ticks=0;
int current_cnt=0;
bool event_condition=false;
static volatile int lost_ticks=0;
static volatile int recover_ticks=0;

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

static const int weights[gray_sensor]={-3500,-2500,-1500,-500,500,1500,2500,3500};

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
int track_count(int values[8])
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
    //特殊路况 zhijiaowan
    if(center_gray&&right_count>=3&&left_count==0)
    {
        return pattern_r90;
    }
    if (center_gray&&right_count>=1&&left_count==0)
    {
        return pattern_rightbranch;
    }
    if(center_gray&&left_count>=1&&right_count==0)
    {
        return pattern_leftbranch;
    }
    return pattern_line;
}

trackstate current_state=state_line;
void track_controll(void)
{
    int sensors[gray_sensor]={0};
    int error=0;
    int correction=0;

    trackpattern pattern;

    sensor_read(sensors);
    pattern=track_analyze(sensors,&error);//分析trackpattern状态
   //通过指针传回了实际error,复写了刚刚的int error=0
    state_ticks++;
    bool maxleft=sensors[0]||sensors[1];//最左边两个任一
    bool maxright=sensors[6]||sensors[7];//同


    //嵌套状态，外层：小车当前执行状态
    switch(current_state)
    {
        case state_line:
        if(pattern!=pattern_lost){
            lost_ticks=0;
        }

        if(pattern==pattern_lost){
            recover_ticks=0;
            if(lost_ticks++>=200){
                condition_rst(state_stop);
                set_target(0,0);
            }
            break;
        }
        if(lost_ticks!=0){
            lost_ticks++;
            if(recover_ticks++<3){
                break;
            }
            lost_ticks=0;
            recover_ticks=0;
        }

        switch(pattern)
            {
                case pattern_line:
                correction=line_pid(&control,error);
                set_target(base_pwm,correction);
                break;
                
                case pattern_rightbranch:
                //右边3个任意多个传感器识别到黑线
                    correction=line_pid(&control,error);
                    set_target(base_pwm,correction);

                    if(track_confirm((pattern==pattern_rightbranch),3)){
                        condition_rst(state_c_lockright);
                    }
                break;

                case pattern_leftbranch:
                //左边3个传感器任意多个识别到黑线
                    correction=line_pid(&control,error);
                    set_target(base_pwm,correction);

                    if(track_confirm((pattern==pattern_leftbranch),3)){
                        condition_rst(state_branch_lockleft);
                    }
                break;

                case pattern_r90:

                break;

                case pattern_allblack:

                break;
                case pattern_lost:
                //识别上一次识别情况保持上次方向，阻塞2s，若任无新识别黑线状态，停机
                    if(lost_ticks++>=200){
                        condition_rst(state_stop);
                        set_target(0,0);
                        recover_ticks=0;
                        lost_ticks=0;
                    }
                break;
            }
        break;
        //通过时路口直行
        case state_c_lockstraight1:
            set_target(base_pwm,0);
            if(track_confirm(pattern==pattern_line,3)){
                condition_rst(state_line);
            }
            break;

        //锁死右转，至左侧最外两个任一识别到黑线
        case state_c_lockright:
            set_target(base_pwm,5);
            if(maxleft==1){
                condition_rst(state_stair1);
            }
            break;

        case state_stair1:
            set_target(base_pwm,correction);
            if(track_confirm((center_bianhua(sensors)==1),2)){
                condition_rst(state_teshu_lockleft1);
            }
            else if(track_confirm((pattern==pattern_leftbranch),2)){
                condition_rst(state_branch_lockleft);
            }
            break;

        case state_teshu_lockleft1:
            set_target(base_pwm,-5);
            if(track_confirm((maxright==1),2)){
                condition_rst(state_stair1);
            }
            break;

        case state_branch_lockleft:
            set_target(base_pwm,-5);
            if(track_confirm((maxright==1),2)){
                condition_rst(state_stair2);
            }
            break;

        case state_stair2:
            set_target(base_pwm,correction);
            if(track_confirm((center_bianhua(sensors)==1),2)){
                condition_rst(state_teshu_lockright2);
            }
            break;

        case state_teshu_lockright2:
            if(track_confirm((pattern==pattern_rightbranch),2)){
                set_target(base_pwm,5);
                if(maxleft==1){
                    condition_rst(state_f);
                }
            }
            break;

        case state_f:
            set_target(base_pwm,correction);
            break;

        case state_stop:
            set_target(0,0);
            allstop();
            break;

        case state_fault:
        default: 
            set_target(0,0);
            allstop();
            break;
    }
}

//两次状态确认进入某状态,滤波,require，（状况真假，审计n次）只返回1/0
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
    static bool shangci_init=true;//1为黑色
    bool current_midium=sensors[3]||sensors[4];
    bool change=shangci_init&&!current_midium;
    shangci_init=current_midium;
    return change;
}

//切换状态清0
extern void condition_rst(trackstate next)
{
    current_state=next;
    state_ticks=0;
    current_cnt=0;
    event_condition=false;

    trackpid_rst(&control,0);
}

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

// //直角，避障90°转弯
// void set_target90r()


