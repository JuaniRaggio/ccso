#pragma once

#include "game_state.h"
#include "game_sync.h"
#include <stdint.h>

typedef struct {
   game_state_t *state;
   game_sync_t *sync;

   size_t reference_count;
} game_t;

typedef enum {
   master,
   player,
   view,
   total_entities,
} entity_t;

game_t new_game(char *argv[], int argc, entity_t who);
game_t game_connect(uint32_t w, uint32_t h);
void game_disconnect(game_t);
void game_end(game_t);
