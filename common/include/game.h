#pragma once

#include <stdint.h>

typedef struct game_cdt_t *game_t;

game_t game_init(char *argv[], int argc);
game_t game_connect(uint32_t w, uint32_t h);
void game_disconnect(game_t);
void game_end(game_t);
