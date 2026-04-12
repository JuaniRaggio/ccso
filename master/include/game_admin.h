#pragma once

#include "game_state.h"
#include "player_protocol.h"
#include <game.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
    pid_t player_pid;
    const char name[MAX_NAME_LENGTH];
} player_registration_requirements_t;

void game_state_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed, int8_t players);

bool game_register_player(player_t current_players[MAX_PLAYERS], size_t idx,
                          player_registration_requirements_t to_register);

size_t game_register_all(player_t current_players[MAX_PLAYERS],
                         player_registration_requirements_t to_register[MAX_PLAYERS]);

bool is_move_allowed(game_state_t *state, uint16_t vertical_coord, uint16_t horizontal_coord);

void apply_move(game_state_t *state, uint16_t vertical_coord, uint16_t horizontal_coord, int8_t playerId);

void register_move(game_state_t *state, bool is_valid_move, int8_t playerId);

void process_player_move(game_state_t *state, uint8_t player_idx, direction_wire_t direction);

bool any_player_alive(game_state_t *state);

void place_players_on_board(game_state_t *state);
