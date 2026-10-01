#ifndef LINE_FOLLOW_H_
#define LINE_FOLLOW_H_

#include <stdint.h>

#include "grayscale_sensor.h"

typedef enum {
    LINE_MODE_IDLE = 0,
    LINE_MODE_NORMAL,
    LINE_MODE_RIGHT_ONCE
} LineFollow_Mode;

typedef enum {
    LINE_STAGE_WAIT_RIGHT = 0,
    LINE_STAGE_DONE
} LineFollow_Stage;

typedef enum {
    LINE_STATE_STOPPED = 0,
    LINE_STATE_NORMAL,
    LINE_STATE_LOST,
    LINE_STATE_RIGHT_LOCK,
    LINE_STATE_LEFT_LOCK,
    LINE_STATE_GYRO_RIGHT,
    LINE_STATE_GYRO_LEFT,
    LINE_STATE_FAULT
} LineFollow_State;

void LineFollow_Init(void);
void LineFollow_SelectMode(LineFollow_Mode mode);
void LineFollow_Update(void);
void LineFollow_Stop(void);

#endif /* LINE_FOLLOW_H_ */
