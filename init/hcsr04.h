#ifndef HCSR04_H_
#define HCSR04_H_

#include <stdint.h>

/* Call after SYSCFG_DL_init(): starts automatic ranging every 100 ms. */
void HCSR04_Init(void);
/* Latest distance in cm; -1.0f before first result or on timeout/out of range. */
float HCSR04_GetDistance(void);

#endif
