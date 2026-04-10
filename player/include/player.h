#include "game.h"
#include <semaphore.h>
#include <stdint.h>

#include <game_sync.h>
#include <player_movement.h>

void start_reading_board(game_sync_t * sync);
void stop_reading_bord(game_sync_t * sync);
void start_playing(game_t * game, uint16_t idx);