#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    dir_up = 0,
    dir_up_right,
    dir_right,
    dir_down_right,
    dir_down,
    dir_down_left,
    dir_left,
    dir_up_left,
    dir_count,
} direction_t;

typedef int8_t direction_wire_t;

#define NO_VALID_MOVE ((direction_wire_t)-1)
typedef struct {
    int8_t dx;
    int8_t dy;
} move_delta_t;

extern const move_delta_t move_delta[dir_count];

