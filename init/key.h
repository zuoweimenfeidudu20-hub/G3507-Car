#ifndef KEY_H
#define KEY_H

#include <stdint.h>

/* 按键编号 */
typedef enum
{
    key1,
    key2,
    key3,
    key4,
    KEY_ID_COUNT
}key;

/* 每个按键的四种消抖状态 */
typedef enum
{
    KEY_STATE_RELEASED = 0,        // 稳定松开
    KEY_STATE_PRESS_DEBOUNCE,      // 按下消抖
    KEY_STATE_PRESSED,             // 稳定按下
    KEY_STATE_RELEASE_DEBOUNCE     // 松开消抖
} KeyState;

/* 对外提供的按键事件 */
typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESS,
    KEY_EVENT_RELEASE
} KeyEvent;

/*
 * 在 SYSCFG_DL_init() 之后调用。
 */
void Key_Init(void);

/*
 * 每 10 ms 调用一次。
 * 放入 pid_timer_INST_IRQHandler()。
 */
void Key_Tick10ms(void);

/*
 * 从 GROUP1_IRQHandler() 中调用。
 * 只处理属于按键的 GPIO 中断标志。
 */
void Key_GPIOInterruptHandler(void);

/*
 * 获取并清除某按键的事件。
 */
KeyEvent Key_GetEvent(KeyId id);

/*
 * 获取经过消抖后的稳定状态：
 * 0 = 松开
 * 1 = 按下
 */
uint8_t Key_IsPressed(KeyId id);

#endif