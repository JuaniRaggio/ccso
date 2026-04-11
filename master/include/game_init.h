#pragma once

#include <game.h>
#include <stdint.h>

void game_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed, int8_t players_count,
               const char *players[]);