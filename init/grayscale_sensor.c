#include "ti_msp_dl_config.h"
#include "grayscale_sensor.h"

typedef struct {
    GPIO_Regs *port;
    uint32_t pin;
} Grayscale_Pin;

/* The line follower numbers sensors from left to right. The board labels
 * them GRAY8 (left) through GRAY1 (right). */
static const Grayscale_Pin s_pins[GRAYSCALE_SENSOR_COUNT] = {
    {Grays_GRAY8_PORT, Grays_GRAY8_PIN},
    {Grays_GRAY7_PORT, Grays_GRAY7_PIN},
    {Grays_GRAY6_PORT, Grays_GRAY6_PIN},
    {Grays_GRAY5_PORT, Grays_GRAY5_PIN},
    {Grays_GRAY4_PORT, Grays_GRAY4_PIN},
    {Grays_GRAY3_PORT, Grays_GRAY3_PIN},
    {Grays_GRAY2_PORT, Grays_GRAY2_PIN},
    {Grays_GRAY1_PORT, Grays_GRAY1_PIN}
};

void Grayscale_Sensor_Init(void)
{
    /* SYSCFG_DL_init configures all eight pins as digital inputs. */
}

void Grayscale_Sensor_ReadAll(uint8_t values[GRAYSCALE_SENSOR_COUNT])
{
    uint32_t i;

    for (i = 0U; i < GRAYSCALE_SENSOR_COUNT; ++i) {
        values[i] = (DL_GPIO_readPins(s_pins[i].port, s_pins[i].pin) != 0U)
                        ? 1U : 0U;
    }
}
