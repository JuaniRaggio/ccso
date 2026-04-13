#pragma once
#include <game_state.h>
#include <player_protocol.h>
#include <stdint.h>

/**
 * @brief: Calculates next move for player
 **/
// int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y);

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y);

int8_t decide_move(game_state_t *state, uint16_t width, uint16_t height, uint16_t idx);

/**
 * @brief: gets a set of available movements (8 bits, each bit is a direction -
 *         0 if invalid)
 **/
uint8_t get_available_moves();
