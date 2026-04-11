#include <game_init.h>
#include <game_state.h>
#include <game_sync.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CELL_REWARD 9
#define MIN_CELL_REWARD 1
#define REWARD_RANGE (MAX_CELL_REWARD - MIN_CELL_REWARD + 1)

static void init_board(game_state_t *state, uint16_t width, uint16_t height, int8_t players_count,
                       const char *players[]) {
    state->width = width;
    state->height = height;
    state->players_count = players_count;

    for (int i = 0; i < players_count; i++) {
        strncpy(state->players[i].name, players[i], MAX_NAME_LENGTH - 1);
        state->players[i].name[MAX_NAME_LENGTH - 1] = '\0';
    }

    state->running = false; // QUE ONDA CON ESTO
    for (uint32_t i = 0; i < (uint32_t)height * width; i++) {
        state->board[i] = (random() % REWARD_RANGE) + MIN_CELL_REWARD;
    }
}


void game_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed, int8_t players_count, const char * players[]) {
    srandom(seed);
    init_board(game->state, width, height, players_count,players);
}
