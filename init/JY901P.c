#include "ti_msp_dl_config.h"
#include "JY901P.h"

#include <stddef.h>
#include <stdint.h>

/* WIT standard serial protocol: every output frame has exactly 11 bytes. */
#define JY901P_FRAME_HEADER       (0x55U)
#define JY901P_GYRO_FRAME_TYPE    (0x52U)
#define JY901P_ANGLE_FRAME_TYPE   (0x53U)
#define JY901P_FRAME_SIZE         (11U)
#define JY901P_CHECKSUM_INDEX     (10U)
#define JY901P_YAW_LOW_INDEX      (6U)
#define JY901P_YAW_HIGH_INDEX     (7U)
#define JY901P_ANGLE_SCALE        (180.0f / 32768.0f)
/* Change 2000.0f if the sensor gyro full-scale setting is changed. */
#define JY901P_GYRO_SCALE         (2000.0f / 32768.0f)

static uint8_t s_frame[JY901P_FRAME_SIZE];
static uint8_t s_frame_index;

/* The ISR stores the protocol's signed 16-bit value; conversion stays outside
 * the interrupt so the interrupt path remains short and deterministic. */
static volatile int16_t s_yaw_raw;
static volatile uint8_t s_yaw_valid;
static volatile uint8_t s_yaw_unread;
static volatile int16_t s_gyro_z_raw;
static volatile uint8_t s_gyro_z_valid;

static bool JY901P_IsStandardFrameType(uint8_t type)
{
    return (((type >= 0x50U) && (type <= 0x5AU)) || (type == 0x5FU));
}

static bool JY901P_ChecksumIsValid(const uint8_t *frame)
{
    uint8_t checksum = 0U;
    uint8_t i;

    for (i = 0U; i < JY901P_CHECKSUM_INDEX; ++i) {
        checksum = (uint8_t)(checksum + frame[i]);
    }

    return (checksum == frame[JY901P_CHECKSUM_INDEX]);
}

static void JY901P_ProcessReceivedByte(uint8_t byte)
{
    if (s_frame_index == 0U) {
        if (byte == JY901P_FRAME_HEADER) {
            s_frame[0] = byte;
            s_frame_index = 1U;
        }
        return;
    }

    if (s_frame_index == 1U) {
        if (JY901P_IsStandardFrameType(byte)) {
            s_frame[1] = byte;
            s_frame_index = 2U;
        } else if (byte == JY901P_FRAME_HEADER) {
            /* The new byte may be the real header after line noise. */
            s_frame[0] = byte;
        } else {
            s_frame_index = 0U;
        }
        return;
    }

    s_frame[s_frame_index++] = byte;
    if (s_frame_index < JY901P_FRAME_SIZE) {
        return;
    }

    if (JY901P_ChecksumIsValid(s_frame)) {
        if (s_frame[1] == JY901P_ANGLE_FRAME_TYPE) {
            uint16_t yaw_bits = (uint16_t)s_frame[JY901P_YAW_LOW_INDEX] |
                                ((uint16_t)s_frame[JY901P_YAW_HIGH_INDEX] << 8U);

            s_yaw_raw = (int16_t)yaw_bits;
            s_yaw_valid = 1U;
            s_yaw_unread = 1U;
        } else if (s_frame[1] == JY901P_GYRO_FRAME_TYPE) {
            uint16_t gyro_z_bits = (uint16_t)s_frame[JY901P_YAW_LOW_INDEX] |
                                   ((uint16_t)s_frame[JY901P_YAW_HIGH_INDEX] << 8U);

            s_gyro_z_raw = (int16_t)gyro_z_bits;
            s_gyro_z_valid = 1U;
        }
    }

    s_frame_index = 0U;
}

static bool JY901P_CopyYawRaw(int16_t *yaw_raw, bool consume)
{
    uint32_t interrupt_state;
    bool available;

    if (yaw_raw == NULL) {
        return false;
    }

    interrupt_state = __get_PRIMASK();
    __disable_irq();

    available = (s_yaw_valid != 0U);
    if (consume) {
        available = available && (s_yaw_unread != 0U);
    }

    if (available) {
        *yaw_raw = s_yaw_raw;
        if (consume) {
            s_yaw_unread = 0U;
        }
    }

    if (interrupt_state == 0U) {
        __enable_irq();
    }

    return available;
}

void JY901P_Init(void)
{
    uint32_t interrupt_state = __get_PRIMASK();

    __disable_irq();
    s_frame_index = 0U;
    s_yaw_raw = 0;
    s_yaw_valid = 0U;
    s_yaw_unread = 0U;
    s_gyro_z_raw = 0;
    s_gyro_z_valid = 0U;

    /* Discard bytes received before the parser and interrupt were ready. */
    while (!DL_UART_Main_isRXFIFOEmpty(UART_JY901P_INST)) {
        (void)DL_UART_Main_receiveData(UART_JY901P_INST);
    }

    DL_UART_Main_clearInterruptStatus(
        UART_JY901P_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART_JY901P_INST_INT_IRQN);
    NVIC_SetPriority(UART_JY901P_INST_INT_IRQN, 2U);
    DL_UART_Main_enableInterrupt(
        UART_JY901P_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_EnableIRQ(UART_JY901P_INST_INT_IRQN);

    if (interrupt_state == 0U) {
        __enable_irq();
    }
}

bool JY901P_GetYaw(float *yaw_degrees)
{
    int16_t yaw_raw;

    if ((yaw_degrees == NULL) || !JY901P_CopyYawRaw(&yaw_raw, false)) {
        return false;
    }

    *yaw_degrees = (float)yaw_raw * JY901P_ANGLE_SCALE;
    return true;
}

bool JY901P_GetGyroZ(float *gyro_z_dps)
{
    int16_t gyro_z_raw;
    uint32_t interrupt_state;
    bool valid;

    if (gyro_z_dps == NULL) {
        return false;
    }

    interrupt_state = __get_PRIMASK();
    __disable_irq();
    valid = (s_gyro_z_valid != 0U);
    gyro_z_raw = s_gyro_z_raw;
    if (interrupt_state == 0U) {
        __enable_irq();
    }

    if (!valid) {
        return false;
    }

    *gyro_z_dps = (float)gyro_z_raw * JY901P_GYRO_SCALE;
    return true;
}

bool JY901P_TakeYaw(float *yaw_degrees)
{
    int16_t yaw_raw;

    if ((yaw_degrees == NULL) || !JY901P_CopyYawRaw(&yaw_raw, true)) {
        return false;
    }

    *yaw_degrees = (float)yaw_raw * JY901P_ANGLE_SCALE;
    return true;
}

void UART_JY901P_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_JY901P_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            while (!DL_UART_Main_isRXFIFOEmpty(UART_JY901P_INST)) {
                JY901P_ProcessReceivedByte(
                    (uint8_t)DL_UART_Main_receiveData(UART_JY901P_INST));
            }
            break;

        default:
            break;
    }
}
