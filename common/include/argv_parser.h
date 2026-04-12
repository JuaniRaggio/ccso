#pragma once

#include <stdbool.h>
#include <stdint.h>

bool parse_board_args(char *argv[], uint16_t *out_width, uint16_t *out_height);
