#include "argv_parser.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool parse_uint16(const char *str, uint16_t *out) {
    char *endptr = (char *)str + strlen(str);
    errno = 0;
    long long val = strtoll(str, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        return false;
    }
    *out = (uint16_t)val;
    return true;
}

bool parse_board_args(char *argv[], uint16_t *out_width, uint16_t *out_height) {
    return parse_uint16(argv[1], out_width) && parse_uint16(argv[2], out_height);
}
