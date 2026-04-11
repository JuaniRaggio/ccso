#include "error_management.h"
#include "game.h"
#include <game_admin.h>
#include <game_state.h>
#include <game_sync.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

bool game_register_player(player_t current_players[MAX_PLAYERS], size_t idx,
                          player_registration_requirements_t to_register) {
    // If player at index already exists, we override it
    if (idx >= MAX_PLAYERS || to_register.name[0] == '\0') {
        manage_error(__FILE__, __func__, __LINE__, invalid_argument_error);
        return false;
    }
    current_players[idx] = (player_t){
        .player_id = to_register.player_pid,
        .state = true,
        .invalid_moves = 0,
        .valid_moves = 0,
    };
    strncpy(current_players[idx].name, to_register.name, MAX_NAME_LENGTH - 1);
    current_players[idx].name[MAX_NAME_LENGTH - 1] = '\0'; // JIC
    return true;
}

size_t game_register_all(player_t current_players[MAX_PLAYERS],
                         player_registration_requirements_t to_register[MAX_PLAYERS]) {
    size_t registered = 0;
    if (to_register == NULL) {
        return manage_error(__FILE__, __func__, __LINE__, invalid_argument_error);
    }
    for (uint_fast8_t i = 0; i < MAX_PLAYERS; ++i) {
        registered += game_register_player(current_players, i, to_register[i]) ? 1 : 0;
    }
    return registered;
}
