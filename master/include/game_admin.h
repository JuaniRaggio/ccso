#pragma once

#include "game_state.h"
#include <game.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/_types/_pid_t.h>

void game_state_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed, int8_t players);

bool game_register_player(game_t *game, size_t idx, pid_t pid, const char name[MAX_NAME_LENGTH]);
