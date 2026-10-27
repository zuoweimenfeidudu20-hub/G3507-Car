#ifndef KEY_H_
#define KEY_H_

#include <stdint.h>

//应用层
typedef enum
{
    key1_putong,
    key2_teshu,
    key3_baidong,
    key4_idle,
}keymode;

typedef enum
{
    id1,
    id2,
    id3,
    id4,
}keyid;


int Key_ReadPress(keyid id);
void key_selectmode(keyid id);
void key_tick30ms();
void key_init(void);
void keyscan(void);
keymode key_getmode(void);

#endif
