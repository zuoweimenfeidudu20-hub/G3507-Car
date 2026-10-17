#include "ti_msp_dl_config.h"
#include "key.h"
#include "grayscale_sensor.h"
#include "motor.h"
#define KEY_DEBOUNCE_TICKS (3)

typedef struct
{
    GPIO_Regs *port;
    uint32_t pin;
}keypin;

static const keypin key_pins[4] = {
    {key_PIN_key1_PORT, key_PIN_key1_PIN},
    {key_PIN_key2_PORT, key_PIN_key2_PIN},
    {key_PIN_key3_PORT, key_PIN_key3_PIN},
    {key_PIN_key4_PORT, key_PIN_key4_PIN}
};

//硬件层状态机,处理是否按下的状态
typedef enum
{
    state_idle,             // 空闲待机
    state_press_debounce,          // 按下消抖
    state_press,          //稳定按下
    state_release     //松开消抖
}keystate;
//确认按键状态维持时间，防止误触
typedef struct
{
    volatile keystate state;
    int ticks;
}keycontrol;

static keycontrol keys[4];//4个按键各自保存一个状态
volatile keymode mode=key_idle;

//1松开，0按下
int Key_ReadPress(keyid id)
{
    if(DL_GPIO_readPins(key_pins[id].port, key_pins[id].pin) == 0){
        return 0;
    }else{
        return 1;
    }
}

//连接应用层
void key_selectmode(keyid id)
{
    if(id==id1){
        mode=key1_putong;
    }else if(id ==id2){
        mode=key2_teshu;
    }else if(id==id3){
        mode=key3_baidong;
    }else if(id==id4){
        mode=key_idle;
    }
}

void key_tick30ms()
{
    int i;
    for(i=0;i<4;i++){
        keyid id=i;
        int pressed=Key_ReadPress(id);//1松开，0按下
        switch(keys[i].state){
                case state_idle:
                    if(pressed==0){
                        keys[i].state=state_press_debounce;
                        keys[i].ticks=1;
                    }
                break;

                case state_press_debounce:
                    if(pressed==1){
                        keys[i].state=state_idle;
                        keys[i].ticks=0;
                    }
                    else{
                        keys[i].ticks++;
                        if(keys[i].ticks>=30){
                            keys[i].state=state_press;
                            keys[i].ticks=0;

                            key_selectmode(id);
                        }
                    }
                break;
                
                case state_press:
                    if(pressed==1){
                        keys[i].state=state_release;
                        keys[i].ticks=0;
                    }
                break;
                    
                case state_release:
                    if(pressed==0){
                        keys[i].state=state_press;
                        keys[i].ticks=0;
                    }
                    else{
                        keys[i].ticks++;
                        if(keys[i].ticks>=30){
                            keys[i].state=state_idle;
                            keys[i].ticks=0;
                        }
                    }
                break;
            default:
                    keys[i].state=state_idle;
                    keys[i].ticks=0;
            break;
        }
    }
}


void key_init(void)
{
    mode=key_idle;
    for(int i=0;i<4;i++){
        keyid id=i;
        if(Key_ReadPress(id)==0){
            keys[i].state=state_press;
        }else{
            keys[i].state=state_idle;
        }//轮询保存当前按键状态
        keys[i].ticks=0;
    }
}
//根据当前
void keyscan(void)
{
    int i;
    for(i=0;i<4;i++){
        keyid id=i;
        int press=Key_ReadPress(id);
        switch(keys[i].state){
            case state_idle:
                set_target(0,0);
                allstop();

            case state_press:

        }
    }
}

keymode key_getmode(void)
{
    return mode;
}

void key_timer_INST_IRQHandler()
{
    static int ticks=0;
    if (DL_TimerG_getPendingInterrupt(key_timer_INST) ==
        DL_TIMERG_IIDX_ZERO) {
        if(ticks++>=30){
            ticks=0;
            keyscan();
        }
    }
}
