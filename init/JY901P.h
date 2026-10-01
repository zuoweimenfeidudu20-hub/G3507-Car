#ifndef JY901P_H_
#define JY901P_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start receiving JY901P data on the SysConfig UART_JY901P port.
 *
 * Call this once after SYSCFG_DL_init().  The configured serial format is
 * 9600 baud, 8 data bits, no parity and 1 stop bit.
 */
void JY901P_Init(void);

/**
 * @brief Read the latest valid yaw angle.
 *
 * @param yaw_degrees Destination for yaw in degrees, in the JY901P protocol
 *                    range [-180, 180).
 * @return true after at least one checksum-valid angle frame has arrived;
 *         false if yaw_degrees is NULL or no valid angle is available yet.
 *
 * Reading does not consume the sample.  Repeated calls return the latest yaw.
 */
bool JY901P_GetYaw(float *yaw_degrees);

/**
 * @brief Read the latest Z-axis angular velocity.
 *
 * The default JY901P gyroscope range is assumed to be +/-2000 deg/s.
 *
 * @param gyro_z_dps Destination for angular velocity in degrees/second.
 * @return true after at least one checksum-valid gyro frame has arrived.
 */
bool JY901P_GetGyroZ(float *gyro_z_dps);

/**
 * @brief Read the latest yaw only when it has changed since the previous call.
 *
 * @param yaw_degrees Destination for yaw in degrees.
 * @return true when a new checksum-valid angle frame was copied; false when
 *         there is no unread sample or yaw_degrees is NULL.
 */
bool JY901P_TakeYaw(float *yaw_degrees);

#ifdef __cplusplus
}
#endif

#endif /* JY901P_H_ */
