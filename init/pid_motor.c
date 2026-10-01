#include "ti_msp_dl_config.h"
#include "motor.h"
#include "pid_motor.h"

typedef struct {
    int32_t integral;
    int32_t previous_error;
    int32_t previous_target;
} PIDMotor_Controller;


extern volatile int right_encoder_count=0;
extern volatile int left_encoder_count=0;
extern volatile int right_error_count = 0;
extern volatile int left_error_count = 0;

 /*索引 = 上次AB * 4 + 本次AB，A为高位、B为低位。 */
static const int8_t encoder_transition[16] = {
      0,  1, -1,  0,
     -1,  0,  0,  1,
      1,  0,  0, -1,
      0, -1,  1,  0
};

int32_t Clamp(int32_t value, int32_t min, int32_t max)
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
}


int pid(PIDMotor_Controller *controller,
                            int32_t target_percent, 
                            int32_t target_counts, 
                            int32_t measured_counts)
{
    int32_t error;
    int32_t derivative;
    int32_t correction;
    int32_t output;
    int32_t previous_integral;

    if (target_percent == 0) {
        ResetController(controller);
        return 0;
    }/*reset*/

    if (Sign(target_percent) != Sign(controller->previous_target)) {
        controller->integral = 0;
        controller->previous_error = 0;
    }/*确保同方向*/

    error = target_counts - measured_counts;/*误差=编码器目标计数-实际计数*/
    derivative = error - controller->previous_error;/*误差改变量=本次误差-上一次误差*/
    previous_integral = controller->integral;
    controller->integral = Clamp(controller->integral + error,
                                 -PID_MOTOR_INTEGRAL_LIMIT,
                                 PID_MOTOR_INTEGRAL_LIMIT);
    /*钳制,错误累积项，为了处理部分微小累积误差*/
     correction = (s_kp * error +
                   s_ki * controller->integral +
                   s_kd * derivative) / PID_MOTOR_GAIN_SCALE;
    /*防止输出pwm过大*/
    if (target_percent > 0) {
        output = Clamp(target_percent + correction, 0, 100);
    } else {
        output = Clamp(target_percent + correction, -100, 0);
    }

    /*抗积分饱和*/
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




void pid_timer_IRQHandler(void)
{
    int i;
    if (DL_TimerG_getPendingInterrupt(pid_timer_INST) == DL_TIMERG_IIDX_ZERO) {
        // 静态变量，用来保存上一次（也就是10ms前）的编码器总计数值
        static int last_right_count = 0;
        static int last_left_count = 0;
        
        // 1. 获取当前时刻的绝对计数值 (快照读取)
        int current_right = right_encoder_count;
        int current_left = left_encoder_count;
        
    }
}

void GROUP1_IRQHandler(void)
{
    int right_pins = MOTOR_DIR1_CH_1_PIN | MOTOR_DIR1_CH_2_PIN;
    int left_pins = MOTOR_DIR2_CH_11_PIN | MOTOR_DIR2_CH_22_PIN;
    //读取编码器引脚是否有中断待处理 pending=0无中断//
    int pending = DL_GPIO_getEnabledInterruptStatus(
        GPIOA, right_pins | left_pins);

    if (pending != 0) {
        //清除中断标志//
        DL_GPIO_clearInterruptStatus(GPIOA, pending);
        int levels = DL_GPIO_readpins(GPIOA, right_pins | left_pins);


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
        // 组合成 4 bit 索引，并查表更新计数 //
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
        // 组合成 4 bit 索引，并查表更新计数 //
        int left = (history_left << 2) | current_left;
        left_encoder_count += encoder_transition[left];
        
        history_left = current_left;
        } 
}
