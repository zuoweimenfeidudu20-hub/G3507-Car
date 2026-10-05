#include "ti_msp_dl_config.h"
#include "motor.h"
#include "pid_motor.h"
#include "stdio.h"
#include "stdint.h"

volatile int32_t right_encoder_count = 0;
volatile int32_t left_encoder_count = 0;
volatile int right_error_count = 0;
volatile int left_error_count = 0;
volatile int right_count_10ms = 0;
volatile int left_count_10ms = 0;
volatile int right_error = 0;
volatile int left_error = 0;

 /*索引 = 上次AB * 4 + 本次AB，A为高位、B为低位。 */
static const int8_t encoder_transition[16] = {
      0,  1, -1,  0,
     -1,  0,  0,  1,
      1,  0,  0, -1,
      0, -1,  1,  0
};

extern int32_t Clamp(int32_t value, int32_t min, int32_t max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

int32_t Absolute(int32_t value)
{
    if(value < 0)
    {
        return -value;
    } 
    else
    {
        return value;
    }
}

int Sign(int32_t value)
{
    if(value > 0){
        return 1;
    }else if(value <0){
        return -1;
    }else{
        return 0;
    }
}

//目标值百分比换算为计数值
int32_t PercentToCounts(int32_t percent)
{
    int32_t Encoder_counts;

    percent = Clamp(percent, -100, 100);
    Encoder_counts = (Absolute(percent) * max_count_10ms + 50) / 100;

    if ((percent != 0) && (Encoder_counts == 0)) {
        Encoder_counts = 1;
    }
    if(percent < 0)
    {
        return(-Encoder_counts);
    }
    return Encoder_counts;
}

PIDMotor_Controller left_pid={0,0,0};
PIDMotor_Controller right_pid={0,0,0};

//pid返回也是pwm
int pid_motor(PIDMotor_Controller *controller,
                            int target_percent, 
                            int target_counts, 
                            int measured_counts)
{
    int error;
    int derivative;
    int correction;
    int output;
    int previous_integral;

    if (Sign(target_percent) != Sign(controller->previous_target)) {
        controller->integral = 0;
        controller->previous_error = 0;
    }//确保同方向

    error = target_counts - measured_counts;//误差=编码器目标计数-实际计数
    derivative = error - controller->previous_error;//误差改变量=本次误差-上一次误差
    previous_integral = controller->integral;
    controller->integral = Clamp(controller->integral + error,
                                 -integral_limit,
                                 integral_limit);
    //钳制,错误累积项，处理部分微小累积误差
     correction = (motor_kp * error +
                   motor_ki * controller->integral +
                   motor_kd * derivative) / pid_motor_gain_scale;

    //防止输出pwm过大
    if (target_percent > 0) {
        output = Clamp(target_percent + correction, 0, 100);
    }else if(target_percent<0){
        output = Clamp(target_percent + correction, -100, 0);
    }else{
        pidmotor_rst(controller);
    }

    //抗积分饱和
    if (((target_percent > 0) &&
         (((output == 100) && (error > 0)) ||
          ((output == 0) && (error < 0)))) ||
        ((target_percent < 0) &&
         (((output == -100) && (error < 0)) ||
          ((output == 0) && (error > 0))))) {
        controller->integral = previous_integral;
    }

    controller->previous_error = error;
    controller->previous_target = target_percent;
    return output;
}

void pidmotor_rst(PIDMotor_Controller *controller)
{
    controller->integral=0;
    controller->previous_error=0;
    controller->previous_target=0;
}

volatile int init_rightpwm=35;
volatile int init_leftpwm=35;

volatile int right_output=0;
volatile int left_output=0;

//10ms中断进入计算编码器计数
void pid_timer_INST_IRQHandler(void)
{
if (DL_TimerG_getPendingInterrupt(pid_timer_INST) == DL_TIMERG_IIDX_ZERO)
    {
        //禁止编码器中断
        NVIC_DisableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
        //获取当前时刻的绝对计数值 (快照读取)
        right_count_10ms = right_encoder_count;
        left_count_10ms = left_encoder_count;

        left_error=left_error_count;
        right_error=right_error_count;

        right_encoder_count=0;
        left_encoder_count=0;
        left_error_count=0;
        right_error_count=0;

        //开启
        NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
        
        right_output=pid_motor(&right_pid,
                        init_rightpwm,
                        PercentToCounts(init_rightpwm),
                        right_count_10ms*encoder_right_dir);
        left_output=pid_motor(&left_pid,
                        init_leftpwm,
                        PercentToCounts(init_leftpwm),
                        left_count_10ms*encoder_left_dir);

        rightmotor_output(right_output);
        leftmotor_output(left_output);

    }
}

void GROUP1_IRQHandler(void)
{
    int right_pins = MOTOR_DIR1_CH_1_PIN | MOTOR_DIR1_CH_2_PIN;
    int left_pins = MOTOR_DIR2_CH_11_PIN | MOTOR_DIR2_CH_22_PIN;
    //读取编码器引脚是否有中断待处理 pending=0无中断
    int pending = DL_GPIO_getEnabledInterruptStatus(
        GPIOA, right_pins | left_pins);

    if (pending != 0) {
        //清除中断标志
        DL_GPIO_clearInterruptStatus(GPIOA, pending);
        int levels = DL_GPIO_readPins(GPIOA, right_pins | left_pins);


        int current_right=0;
        static int history_right=0;
        if((levels & MOTOR_DIR1_CH_1_PIN)!=0){
            current_right |= 2;
        }
        if ((levels & MOTOR_DIR1_CH_2_PIN) != 0) {
            current_right |= 1; 
        }
        if ((history_right ^ current_right) == 3) {
            right_error_count++; // 非法跳变计数+1
        }
        // 组合成 4 bit 索引，并查表更新计数 
        int right = (history_right << 2) | current_right;
        right_encoder_count += encoder_transition[right];
        
        history_right = current_right;
     

        
        int current_left=0;
        static int history_left=0;
        if((levels & MOTOR_DIR2_CH_11_PIN)!=0){
            current_left |= 2;
        }
        if ((levels & MOTOR_DIR2_CH_22_PIN) != 0) {
            current_left |= 1; 
        }
        if ((history_left ^ current_left) == 3) {
            left_error_count++; // 非法跳变计数+1
        }
        // 组合成 4 bit 索引，并查表更新计数 
        int left = (history_left << 2) | current_left;
        left_encoder_count += encoder_transition[left];
        
        history_left = current_left;
        } 
}
