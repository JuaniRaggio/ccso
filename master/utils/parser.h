#pragma once

#include <game_state.h>
#include <stdint.h>

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
   invalid_integer_type = success << 1,
   invalid_view_path = invalid_integer_type << 1,
   invalid_player_count = invalid_view_path << 1,
   invalid_width_limit = invalid_player_count << 1,
   invalid_height_limit = invalid_width_limit << 1,
} parameter_status_t;

parameter_status_t parse(int argc, char *argv[], parameters_t *parameters);
