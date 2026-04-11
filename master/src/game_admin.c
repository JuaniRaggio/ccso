#include <game_admin.h>
#include <game_state.h>
#include <game_sync.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_CELL_REWARD 9
#define MIN_CELL_REWARD 1
#define REWARD_RANGE (MAX_CELL_REWARD - MIN_CELL_REWARD + 1)

static void board_init(game_state_t *state, uint16_t width, uint16_t height, int8_t players) {
    state->width = width;
    state->height = height;
    state->players_count = players;
    state->running = false;
    for (uint32_t i = 0; i < (uint32_t)height * width; i++) {
        state->board[i] = (random() % REWARD_RANGE) + MIN_CELL_REWARD;
    }
}

void game_state_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed, int8_t players) {
    srandom(seed);
    board_init(game->state, width, height, players);
}
