#pragma once

#include "stefi_board.h"
#include <stdint.h>

/*
 * STSPIN240 brushed-DC motor control for the RobOhmobil.
 *
 * Each motor is driven in PHASE/ENABLE style: one PWM line sets speed (duty)
 * and one DIR line sets phase (direction). The board's PWM output compare is
 * active-low, so 0 % duty == motor off; the component hides that detail.
 *
 * board_init() already configures the motor pins and the shared PWM timer, so
 * an app may call motor_drive() directly. motor_init() re-applies that setup
 * for use without the Rohmi board_init() (and is safe to call again).
 */

void motor_init(void);

/*
 * Drive a motor. speed_pct in [-100, 100]: sign selects direction (forward vs.
 * reverse), magnitude selects duty in percent. 0 stops (coast). Values outside
 * the range are clamped.
 */
void motor_drive(stefi_motor_t id, int8_t speed_pct);

/* Stop one motor / both motors (0 % duty, coast). */
void motor_stop(stefi_motor_t id);
void motor_stop_all(void);