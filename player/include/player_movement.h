#pragma once
#include <game_state.h>
#include <stdint.h>

/**
 * @brief: Calculates next move for player
 **/
int8_t compute_next_move(int8_t[], uint16_t, uint16_t);

/**
 * @brief: gets a set of available movements (8 bits, each bit is a direction -
 *         0 if invalid)
 **/
uint8_t get_available_moves();
