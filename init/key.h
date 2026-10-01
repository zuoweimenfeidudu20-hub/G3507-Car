#ifndef KEY_H_
#define KEY_H_
#include <stdint.h>

typedef enum {
    KEY_MODE_IDLE = 0, KEY_MODE_1, KEY_MODE_2, KEY_MODE_3, KEY_MODE_4
} Key_Mode;
extern volatile uint32_t g_key_count;
void Key_Init(void);             /* After SYSCFG_DL_init, before timer start. */
void Key_HandleInterrupt(void);  /* Shared GROUP1 interrupt dispatcher. */
void Key_Scan10ms(void);          /* Exactly once per 10 ms timer interrupt. */
void Key_Task(void);             /* Main loop, nonblocking. */
Key_Mode Key_GetMode(void);      /* Main-loop access. */
/* Returns and clears press events; bit 0..3 correspond to keys 1..4. */
uint8_t Key_TakePresses(void);
#endif
