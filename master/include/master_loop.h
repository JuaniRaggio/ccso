/**
 * @file master_loop.h
 * @brief Main event loop of the master process.
 *
 * Runs a select()-based loop that reads player moves, applies them to
 * the game state, synchronises the view, and enforces the inactivity
 * timeout. The loop exits on signal, timeout, or when all players die.
 */
#pragma once

#include "pipes.h"
#include <game.h>
#include <parser.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * @brief Execute the master's main game loop.
 *
 * Processes rounds of player moves via select(), syncs the view after
 * each round, and monitors the inactivity timeout. Returns when the
 * game ends for any reason.
 *
 * @param game     Game handle (state + sync).
 * @param params   Parsed command-line parameters (timeout, delay, etc.).
 * @param pipes    Pipe array (one per player).
 * @param view_pid PID of the view process, or 0 if no view.
 * @param has_view Pointer to the view-alive flag; set to false if the
 *                 view process exits prematurely.
 * @return         true if the loop was interrupted by a signal (SIGINT/SIGTERM),
 *                 false if the game ended normally.
 */
bool master_run(game_t *game, parameters_t *params, int32_t pipes[][pipe_ends], pid_t view_pid, bool *has_view);
