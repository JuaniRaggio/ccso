#include "game_state.h"
#include "game_sync.h"
#include <game.h>
#include <stddef.h>
#include <sys/mman.h>

typedef struct {
    game_state_t * state;
    game_sync_t * sync;

    size_t reference_count;
} game_cdt_t;

game_t new_game(char * argv[], int argc) {
    game_t game = malloc(1/* TODO complete this */);

    game->reference_count = 1;
}

game_t game_connect(uint32_t w, uint32_t h) {}

void game_disconnect(game_t game) {
    shm_unlink(game_state_memory_name);
    shm_unlink(game_sync_memory_name);

    // Ownership of this memory depends on how many references are left
    if (--game->reference_count == 0) {
    }
    game->state = NULL;
    game->sync = NULL;
}


void delete_game(game_t game) {
  game_disconnect(game);
}


void game_end(game_t);


