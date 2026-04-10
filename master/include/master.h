#pragma once

#include <game_state.h>
#include <game_sync.h>
#include <stdint.h>

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width);
void printGameSync(game_sync_t *sync);
