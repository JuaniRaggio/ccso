/**
 * @file player_loop.h
 * @brief Main loop for the player process.
 *
 * After the player attaches to shared memory, this loop repeatedly
 * reads the board under a reader lock, computes the next move via
 * the compiled-in AI strategy, sends the direction through stdout
 * (redirected to the master's pipe), and waits for the next turn token.
 */
#pragma once

#include <game.h>

/**
 * @brief Execute the player's main loop.
 *
 * Locates this process's index in the game state by PID, then enters
 * the wait-compute-send cycle until the game ends, a signal is received,
 * or no valid move remains.
 *
 * @param game Game handle (state + sync) mapped in the player's address space.
 */
void player_run(game_t *game);
