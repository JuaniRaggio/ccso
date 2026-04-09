#pragma once
#include <game_state.h>
#include <stdint.h>

typedef enum {
    DIR_UP = 0,
    DIR_UP_RIGHT = 1,
    DIR_RIGHT = 2,
    DIR_DOWN_RIGHT = 3,
    DIR_DOWN = 4,
    DIR_DOWN_LEFT = 5,
    DIR_LEFT = 6,
    DIR_UP_LEFT = 7,
    DIR_COUNT = 8,
} direction_t;

/**
 * @brief: Calculates next move for player
 **/
int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y);

int8_t decidir_movimiento(game_state_t *state, uint16_t width, uint16_t height, uint16_t idx);

/**
 * @brief: gets a set of available movements (8 bits, each bit is a direction -
 *         0 if invalid)
 **/
uint8_t get_available_moves();
