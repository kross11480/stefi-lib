#pragma once

#include "stefi_board.h"
#include <stdbool.h>

/*
 * Reflectance line-sensor reads for the RobOhmobil.
 *
 * The two sensors (LINE_SENSOR_L, LINE_SENSOR_R) are digital GPIO inputs.
 * board_init() already configures them as inputs; line_sensor_init() re-applies
 * that for use without the Rohmi board_init() (and is safe to call again).
 */

void line_sensor_init(void);

/* Raw pin level: true when the GPIO reads HIGH. */
bool line_sensor_read(stefi_line_sensor_t id);

/* Semantic read: true when the sensor sees the track line. Polarity depends on
 * the reflectance module + line/floor contrast — verify on hardware (see
 * LINE_DETECTED_LEVEL in line_sensor.c). */
bool line_sensor_on_line(stefi_line_sensor_t id);

/*
 * Combined state of both sensors — the input to the line-follow and junction
 * logic. With two binary sensors a junction is inferred from the time profile
 * of these states (e.g. a sustained LINE_STATE_BOTH across a perpendicular
 * line), not read directly.
 */
typedef enum {
    LINE_STATE_LOST = 0,  /* neither sensor on the line */
    LINE_STATE_LEFT,      /* only the left sensor on the line */
    LINE_STATE_RIGHT,     /* only the right sensor on the line */
    LINE_STATE_BOTH,      /* both sensors on the line (junction / crossing) */
} line_state_t;

line_state_t line_sensor_state(void);
