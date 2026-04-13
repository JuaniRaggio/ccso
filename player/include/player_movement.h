/**
 * @file player_movement.h
 * @brief AI strategy interface for computing the next move.
 *
 * The concrete strategy is selected at compile time via preprocessor flags
 * (GREEDY, FLOOD, GREEDY_FLOOD, etc.). All strategies expose the same
 * @ref compute_next_move entry point.
 */
#pragma once
#include <game_state.h>
#include <player_protocol.h>
#include <stdint.h>

/**
 * @brief Compute the next directional move for a player.
 *
 * Analyses the board around the player's current position and returns
 * the best direction according to the compiled-in strategy.
 *
 * @param board  Flat board array (row-major, width * height cells).
 * @param width  Board width in cells.
 * @param height Board height in cells.
 * @param x      Player's current column.
 * @param y      Player's current row.
 * @return       A direction_wire_t in [0, dir_count), or NO_VALID_MOVE
 *               if no reachable cell has a positive reward.
 */
int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y);

/**
 * @brief Compute the next move using the full game state.
 *
 * Wrapper that extracts the player's position from the state and
 * delegates to @ref compute_next_move.
 *
 * @param state  Game state with the board and player array.
 * @param width  Board width in cells.
 * @param height Board height in cells.
 * @param idx    Zero-based player index.
 * @return       A direction_wire_t, or NO_VALID_MOVE.
 */
int8_t decide_move(game_state_t *state, uint16_t width, uint16_t height, uint16_t idx);

/**
 * @brief Get a bitmask of available movement directions.
 *
 * Each bit corresponds to a direction_t value. A set bit (1) indicates
 * the direction leads to a valid, in-bounds cell with a positive reward.
 *
 * @return 8-bit mask where bit i is set if direction i is valid.
 */
uint8_t get_available_moves();
