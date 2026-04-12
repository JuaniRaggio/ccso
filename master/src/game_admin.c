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

static void board_init(game_state_t *state, uint16_t width, uint16_t height, int8_t players);
static inline size_t get_board_offset(game_state_t *state, uint_fast16_t vertical_coord,
                                      uint_fast16_t horizontal_coord);

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
        manage_error(HERE, TRACE_NONE, invalid_argument_error);
        return false;
    }
    current_players[idx] = (player_t){
        .player_id = to_register.player_pid,
        .state = true,
        .score = 0,
        .invalid_moves = 0,
        .valid_moves = 0,
        .x = 0,
        .y = 0,
    };
    strncpy(current_players[idx].name, to_register.name, MAX_NAME_LENGTH - 1);
    current_players[idx].name[MAX_NAME_LENGTH - 1] = '\0';
    return true;
}

size_t game_register_all(player_t current_players[MAX_PLAYERS],
                         player_registration_requirements_t to_register[MAX_PLAYERS]) {
    size_t registered = 0;
    if (to_register == NULL) {
        return manage_error(HERE, TRACE_NONE, invalid_argument_error);
    }
    for (uint_fast8_t i = 0; i < MAX_PLAYERS; ++i) {
        registered += game_register_player(current_players, i, to_register[i]) ? 1 : 0;
    }
    return registered;
}

static inline size_t get_board_offset(game_state_t *state, uint_fast16_t vertical_coord,
                                      uint_fast16_t horizontal_coord) {
    return vertical_coord * state->width + horizontal_coord;
}

bool is_move_allowed(game_state_t *state, uint16_t vertical_coord, uint16_t horizontal_coord) {
    if (vertical_coord >= state->height || horizontal_coord >= state->width) {
        return false;
    }
    int index = vertical_coord * state->width + horizontal_coord;
    if (state->board[index] <= 0) {
        return false;
    }
    return true;
}

void apply_move(game_state_t *state, uint16_t vertical_coord, uint16_t horizontal_coord, int8_t player_id) {
    bool valid_move = true;
    if (!is_move_allowed(state, vertical_coord, horizontal_coord)) {
        valid_move = false;
    } else {
        size_t offset = get_board_offset(state, vertical_coord, horizontal_coord);
        state->players[player_id].score += state->board[offset];
        state->players[player_id].x = horizontal_coord;
        state->players[player_id].y = vertical_coord;
        state->board[offset] = -player_id;
    }
    register_move(state, valid_move, player_id);
}

void register_move(game_state_t *state, const bool is_valid_move, int8_t player_id) {
    if (is_valid_move) {
        state->players[player_id].valid_moves++;
    } else {
        state->players[player_id].invalid_moves++;
    }
}

void process_player_move(game_state_t *state, uint8_t player_idx, direction_wire_t direction) {
    player_t *p = &state->players[player_idx];

    if (!is_valid_direction(direction)) {
        register_move(state, false, player_idx);
        return;
    }

    int16_t new_x, new_y;
    apply_direction(p->x, p->y, direction, &new_x, &new_y);

    apply_move(state, new_y, new_x, player_idx);
}

bool any_player_alive(game_state_t *state) {
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].state)
            return true;
    }
    return false;
}

void place_players_on_board(game_state_t *state) {
    int n = state->players_count;
    int cols = 1;
    while (cols * cols < n)
        cols++;
    int rows = (n + cols - 1) / cols;
    int step_x = state->width / (cols + 1);
    int step_y = state->height / (rows + 1);
    for (int i = 0; i < n; i++) {
        uint16_t x = (uint16_t)((i % cols + 1) * step_x);
        uint16_t y = (uint16_t)((i / cols + 1) * step_y);
        state->players[i].x = x;
        state->players[i].y = y;
        size_t offset = (size_t)y * state->width + x;
        state->board[offset] = -(int8_t)i;
    }
}
