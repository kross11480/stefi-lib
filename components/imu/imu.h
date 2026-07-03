#pragma once

#include "stefi_board.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * LSM6DSO 6-axis IMU (accelerometer + gyroscope) on I2C2 for the RobOhmobil.
 *
 * The board's `imu` config (boards/rohmi_g431/board.c) holds the bus, address
 * and pins. board_init() brings up the I2C2 GPIOs; imu_init() then enables the
 * peripheral and configures the sensor. imu_init() also re-applies the GPIO
 * setup, so it works without the Rohmi board_init() too (and is safe to call
 * again).
 *
 * Default configuration applied by imu_init(): accelerometer at 104 Hz, +/-2 g;
 * gyroscope at 104 Hz, 250 dps. The raw 16-bit readings use these full-scale
 * sensitivities — convert with imu_accel_mg() / imu_gyro_mdps().
 */

/* Raw signed 16-bit per-axis sample (LSB), as read from the device. */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} imu_sample_t;

/*
 * Bring up the bus and configure the sensor. Returns true if the device
 * answers with the expected WHO_AM_I (0x6C); false means the IMU is not
 * responding (check wiring / SA0 address strap in board.c).
 */
bool imu_init(void);

/* WHO_AM_I register (expect IMU_WHO_AM_I_EXPECT). */
uint8_t imu_who_am_i(void);

/* Read the latest accelerometer / gyroscope sample (raw LSB). */
void imu_read_accel(imu_sample_t *accel);
void imu_read_gyro(imu_sample_t *gyro);

/* Read both in one auto-incremented burst (gyro then accel). Either pointer
 * may be NULL to skip storing that half. */
void imu_read(imu_sample_t *accel, imu_sample_t *gyro);

#define IMU_WHO_AM_I_EXPECT 0x6C

/* Scale conversions for the default full-scale settings above.
 * +/-2 g  -> 0.061 mg/LSB ; 250 dps -> 8.75 mdps/LSB. */
static inline int32_t imu_accel_mg(int16_t raw)   { return (int32_t) raw * 61 / 1000; }
static inline int32_t imu_gyro_mdps(int16_t raw)  { return (int32_t) raw * 875 / 100; }
