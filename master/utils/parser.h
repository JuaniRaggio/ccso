#pragma once

#include <stdint.h>
#include <game_state.h>

typedef struct {
    uint64_t players_count;
    uint64_t seed;
    uint64_t width;
    uint64_t height;
    uint64_t delay;
    uint64_t timeout;
    char *view_path;
    char *players_paths[MAX_PLAYERS];
} parameters_t;

typedef enum {
    success = 0x0,
    invalid_integer_type = 0x1,
    exceeded_player_limit = invalid_integer_type << 1,
    unknown_optional_flag = exceeded_player_limit << 1,
    overflow = unknown_optional_flag << 1,
} parameter_status_t;

static inline void parse_argument(int opt, parameters_t *parameters, parameter_status_t *status);

parameter_status_t parse(int argc, char *argv[], parameters_t *parameters);
