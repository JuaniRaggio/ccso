#include <game_init.h>
#include <game_state.h>
#include <game_sync.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_CELL_REWARD 9
#define MIN_CELL_REWARD 1
#define REWARD_RANGE (MAX_CELL_REWARD - MIN_CELL_REWARD + 1)

static void init_board(game_state_t *state, uint16_t width, uint16_t height) {
    state->width = width;
    state->height = height;
    state->players_count = 0;
    state->running = false;
    for (uint32_t i = 0; i < (uint32_t)height * width; i++) {
        state->board[i] = (random() % REWARD_RANGE) + MIN_CELL_REWARD;
    }
}

static void init_sync(game_sync_t *sync) {
    sem_init(&sync->pending_changes_to_show, 1, 0);
    sem_init(&sync->printing, 1, 0);
    sem_init(&sync->master_writing, 1, 1);
    sem_init(&sync->gamestate_mutex, 1, 1);
    sem_init(&sync->readers_count_mutex, 1, 1);
    sync->readers_count = 0;
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        sem_init(&sync->player_may_send_movement[i], 1, 1);
    }
}

void game_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed) {
    srandom(seed);
    init_board(game->state, width, height);
    init_sync(game->sync);
}
