/**
 * @file pipes.h
 * @brief Pipe and process management for the master process.
 *
 * Creates the pipe array used for player-to-master communication,
 * forks and exec's the player and view child processes, and provides
 * helpers for fd_set management and cleanup.
 */
#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/select.h>
#include "game_state.h"

/**
 * @brief Indices into a two-element pipe descriptor array.
 */
typedef enum {
    invalid_pipe = -1,
    pipe_reader = 0,
    pipe_writer,
    pipe_ends,
} pipe_users_t;

/**
 * @brief Create one pipe per player.
 *
 * @param[out] pipes        Array of pipe descriptor pairs to populate.
 * @param      playersCount Number of pipes to create.
 */
void create_pipes(int pipes[][pipe_ends], int playersCount);

/**
 * @brief Fork and exec the view process.
 *
 * The child calls execve with the board dimensions as positional arguments.
 *
 * @param view_path Path to the view binary.
 * @param width     Board width as a string.
 * @param height    Board height as a string.
 * @return          PID of the view child process.
 */
pid_t fork_view(const char *view_path, const char *width, const char *height);

/**
 * @brief Fork and exec all player processes.
 *
 * Each child redirects its pipe write end to stdout via dup2 and then
 * calls execve. After all forks, the master's per-player PIDs are
 * stored in @c game_state->players[i].player_id.
 *
 * @param pipes        Pipe array (one per player).
 * @param playersCount Number of players to fork.
 * @param game_state   Game state where PIDs will be recorded.
 * @param player_paths Array of player binary paths.
 * @param width        Board width as a string (forwarded to players).
 * @param height       Board height as a string (forwarded to players).
 */
void fork_players(int pipes[][pipe_ends], int playersCount, game_state_t *game_state, const char *player_paths[],
                  const char *width, const char *height);

/**
 * @brief Close a specific end of all pipes except one.
 *
 * Used by child processes to close file descriptors they do not need.
 *
 * @param pipes              Pipe descriptor array.
 * @param pipe_count         Total number of pipes.
 * @param dont_close_this_pipe  Index of the pipe to keep open, or
 *                              @ref invalid_pipe to close all.
 * @param user_to_close      Which end (@ref pipe_reader or @ref pipe_writer) to close.
 */
void close_other_pipes(int32_t pipes[][pipe_ends], uint32_t pipe_count, ssize_t dont_close_this_pipe,
                       const pipe_users_t user_to_close);

/**
 * @brief Initialise an fd_set with the read ends of all player pipes.
 *
 * @param[out] masterSet  fd_set to populate.
 * @param      pipes      Pipe array.
 * @param      playersCount Number of players.
 * @param[out] maxFd      Receives the highest file descriptor added.
 */
void init_fd_set(fd_set *masterSet, int pipes[][pipe_ends], int playersCount, int *maxFd);

/**
 * @brief Mark a player as dead and remove its pipe from the fd_set.
 *
 * Sets @c player->state to false, clears the fd from @p master_set,
 * and closes the pipe read end.
 *
 * @param player     Player descriptor to update.
 * @param pipes      Pipe array.
 * @param master_set fd_set from which the pipe will be removed.
 * @param idx        Zero-based player index.
 */
void disconnect_player(player_t *player, int32_t pipes[][pipe_ends], fd_set *master_set, int8_t idx);

/**
 * @brief Close the read end of all pipes for players still alive.
 *
 * Called during final cleanup after the game has ended.
 *
 * @param pipes   Pipe array.
 * @param players Player descriptor array.
 * @param count   Number of players.
 */
void close_active_pipes(int32_t pipes[][pipe_ends], player_t players[], int8_t count);
