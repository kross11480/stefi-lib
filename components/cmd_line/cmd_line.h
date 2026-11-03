#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Line-oriented command input for text protocols (UART shell, USB CDC).
 *
 * cmd_line assembles a byte stream into lines ('\n' or '\r' terminated,
 * empty lines skipped, over-long lines dropped) and hands each complete
 * line to a handler. cmd_dispatch() then matches the first word of a line
 * against a table and passes the rest (leading blanks stripped) as args:
 *
 *   static const cmd_entry_t cmds[] = {
 *       {"RUN_START",      on_run},
 *       {"CONFIGURE_FREQ", on_freq},     // "CONFIGURE_FREQ 1000000"
 *   };
 *   static void on_line(const char *line, void *aux) {
 *       if (!cmd_dispatch(cmds, countof(cmds), line, aux)) reply("ERR:UNKNOWN_CMD");
 *   }
 */

typedef void (*cmd_line_handler_t)(const char *line, void *aux);

typedef struct {
    char *buf;
    uint16_t size;          /* including the terminating NUL */
    uint16_t len;
    bool overflow;          /* current line was too long: drop until EOL */
    cmd_line_handler_t fn;
    void *aux;
} cmd_line_t;

void cmd_line_init(cmd_line_t *cl, char *buf, uint16_t size, cmd_line_handler_t fn, void *aux);
void cmd_line_feed(cmd_line_t *cl, const uint8_t *data, uint32_t len);
void cmd_line_reset(cmd_line_t *cl);

typedef void (*cmd_fn_t)(const char *args, void *aux);

typedef struct {
    const char *name;       /* first word of the line, matched exactly */
    cmd_fn_t fn;
} cmd_entry_t;

/** @return false if no entry matched (the line is left to the caller). */
bool cmd_dispatch(const cmd_entry_t *table, size_t n, const char *line, void *aux);

/** Parse an unsigned decimal argument; false (and *out unchanged) if absent/invalid. */
bool cmd_parse_u32(const char *args, uint32_t *out);
