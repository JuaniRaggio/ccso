#pragma once

#include "error_management.h"
#include "game_state.h"
#include "game_sync.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/_types/_pid_t.h>

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

game_t new_game(entity_t who, error_manager_t manage_error, const char *file, const char *func, uint64_t line);
game_t game_connect(uint32_t w, uint32_t h);
bool is_player(game_t *game, pid_t player_id);
void game_disconnect(game_t *);
void game_end(game_t *);
