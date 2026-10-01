#include "delay.h"
#include "ti_msp_dl_config.h"

void delay_ms(uint32_t ms)
{
    while (ms-- > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
    }
}

void delay_us(uint32_t us)
{
    while (us-- > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000000U);
    }
}


void delay_s(uint32_t s)
{
    while (s-- > 0U) {
        delay_cycles(CPUCLK_FREQ / 1U);
    }
}

