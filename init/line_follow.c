#include "line_follow.h"
#include "ti_msp_dl_config.h"
#include "pid_motor.h"

#define BASE_SPEED                 20
#define MAX_CORRECTION              5
#define TURN_CORRECTION            20
#define DEAD_ZONE          600
#define LINE_PD_GAIN           1000
#define LINE_PD_KP                      11
#define LINE_PD_KD                       1

/* One update every 10 ms from the motor-control timer. */
#define LINE_SIDE_CONFIRM_UPDATES       2U


typedef enum {
    TURN_NONE = 0,
    TURN_RIGHT
} TurnLock;

static void ApplySpeeds(int32_t left_speed, int32_t right_speed)
{
    
    PIDMotor_SetTarget(g_line_left_speed, g_line_right_speed);
}

static void ApplyLockedTurn(void)
{
    ApplySpeeds(LINE_BASE_SPEED + LINE_TURN_CORRECTION,
                LINE_BASE_SPEED - LINE_TURN_CORRECTION);
}

static void FollowWeightedLine(uint32_t active_count, int32_t position_sum)
{
    int32_t correction;

    if (active_count == 0U) {
        if (g_line_lost_updates < LINE_LOST_STOP_UPDATES)
            ++g_line_lost_updates;
        if (g_line_lost_updates >= LINE_LOST_STOP_UPDATES) {
            EnterFault();
            return;
        }
        g_line_state = LINE_STATE_LOST;
        PIDMotor_SetTarget(g_line_left_speed, g_line_right_speed);
        return;
    }

    g_line_lost_updates = 0U;
    g_line_error = position_sum / (int32_t)active_count;
    if (g_line_error >= -LINE_ERROR_DEAD_ZONE &&
        g_line_error <= LINE_ERROR_DEAD_ZONE)
        g_line_error = 0;

    if (s_error_initialized == 0U) {
        g_line_derivative = 0;
        s_error_initialized = 1U;
    } else {
        g_line_derivative = g_line_error - s_previous_error;
    }
    s_previous_error = g_line_error;
    correction = (LINE_PD_KP * g_line_error +
                  LINE_PD_KD * g_line_derivative) / LINE_PD_GAIN_SCALE;
    g_line_correction = Clamp(correction, -LINE_MAX_CORRECTION,
                             LINE_MAX_CORRECTION);
    g_line_state = LINE_STATE_NORMAL;
    ApplySpeeds(LINE_BASE_SPEED + g_line_correction,
                LINE_BASE_SPEED - g_line_correction);
}

void LineFollow_Update(void)
{
    static const int32_t position[GRAYSCALE_SENSOR_COUNT] = {
        -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
    };
    uint8_t values[GRAYSCALE_SENSOR_COUNT];
    uint32_t active_count = 0U;
    int32_t position_sum = 0;
    uint8_t right_black = 0U;
    uint8_t left_outer_black;
    uint32_t i;

    if (g_line_mode == LINE_MODE_IDLE || g_line_state == LINE_STATE_FAULT)
        return;

    Grayscale_Sensor_ReadAll(values);
    for (i = 0U; i < GRAYSCALE_SENSOR_COUNT; ++i) {
        g_line_sensor[i] = values[i];
        if (values[i] == GRAYSCALE_BLACK_LEVEL) {
            ++active_count;
            position_sum += position[i];
            if (i >= 5U) right_black = 1U;
        }
    }
    g_line_active_count = active_count;
    left_outer_black = (uint8_t)(values[0] == GRAYSCALE_BLACK_LEVEL ||
                                 values[1] == GRAYSCALE_BLACK_LEVEL);
    if (s_turn_lock == TURN_RIGHT) {
        /* Keep turning until either of the far-left two sensors sees black. */
        if (left_outer_black == 0U) {
            ApplyLockedTurn();
            return;
        }
        s_turn_lock = TURN_NONE;
        g_line_stage = LINE_STAGE_DONE;
        s_previous_error = 0;
        s_error_initialized = 0U;
    }

    if (g_line_mode == LINE_MODE_RIGHT_ONCE &&
        g_line_stage == LINE_STAGE_WAIT_RIGHT) {
        /* Two consecutive 10 ms samples with any of sensors 5..7 black. */
        if (right_black != 0U) {
            if (s_right_hits < LINE_SIDE_CONFIRM_UPDATES) ++s_right_hits;
        } else {
            s_right_hits = 0U;
        }
        if (s_right_hits >= LINE_SIDE_CONFIRM_UPDATES) {
            StartLockedTurn();
            return;
        }
    }

    FollowWeightedLine(active_count, position_sum);
}
