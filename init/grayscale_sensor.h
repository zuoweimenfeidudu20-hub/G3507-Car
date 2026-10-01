#ifndef GRAYSCALE_SENSOR_H_
#define GRAYSCALE_SENSOR_H_

#include <stdint.h>

#define GRAYSCALE_SENSOR_COUNT 8U

/* The new digital sensors output 1 on a black line. */
#define GRAYSCALE_BLACK_LEVEL 1U

void Grayscale_Sensor_Init(void);
void Grayscale_Sensor_ReadAll(uint8_t values[GRAYSCALE_SENSOR_COUNT]);

#endif /* GRAYSCALE_SENSOR_H_ */
