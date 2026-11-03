#include "cmd_line.h"

#include <string.h>

void cmd_line_init(cmd_line_t *cl, char *buf, uint16_t size, cmd_line_handler_t fn, void *aux)
{
    cl->buf = buf;
    cl->size = size;
    cl->fn = fn;
    cl->aux = aux;
    cmd_line_reset(cl);
}

void cmd_line_reset(cmd_line_t *cl)
{
    cl->len = 0;
    cl->overflow = false;
}

void cmd_line_feed(cmd_line_t *cl, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        char ch = (char) data[i];
        if (ch == '\n' || ch == '\r') {
            if (!cl->overflow && cl->len > 0) {
                cl->buf[cl->len] = '\0';
                cl->fn(cl->buf, cl->aux);
            }
            cmd_line_reset(cl);
        } else if (cl->overflow) {
            // discard the rest of an over-long line
        } else if (cl->len < cl->size - 1u) {
            cl->buf[cl->len++] = ch;
        } else {
            cl->overflow = true;
        }
    }
}

bool cmd_dispatch(const cmd_entry_t *table, size_t n, const char *line, void *aux)
{
    size_t word = strcspn(line, " \t");
    for (size_t i = 0; i < n; i++) {
        if (strlen(table[i].name) == word && strncmp(table[i].name, line, word) == 0) {
            const char *args = line + word;
            while (*args == ' ' || *args == '\t') args++;
            table[i].fn(args, aux);
            return true;
        }
    }
    return false;
}

bool cmd_parse_u32(const char *args, uint32_t *out)
{
    uint32_t v = 0;
    const char *p = args;
    if (*p < '0' || *p > '9') return false;
    while (*p >= '0' && *p <= '9') {
        uint32_t d = (uint32_t) (*p - '0');
        if (v > (UINT32_MAX - d) / 10u) return false;   // overflow
        v = v * 10u + d;
        p++;
    }
    if (*p != '\0' && *p != ' ' && *p != '\t') return false;
    *out = v;
    return true;
}
