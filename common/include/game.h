/**
 * @file game.h
 * @brief High-level game lifecycle: creation, teardown, and player queries.
 *
 * Encapsulates the pair of shared memory segments (game state + sync)
 * behind a single @ref game_t handle. Provides factory functions for
 * each entity type (master, player, view), as well as cleanup and
 * simple player-presence queries.
 */
#pragma once

#include "error_management.h"
#include "game_state.h"
#include "game_sync.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/**
 * @brief Identifies the kind of process creating the game handle.
 *
 * The master creates and truncates the shared memory; players and the
 * view attach to an existing segment with read-only (state) or
 * read-write (sync) permissions.
 */
typedef enum {
    master,
    player,
    view,
    total_entities,
} entity_t;

/**
 * @brief Complete game handle combining both shared memory regions.
 */
typedef struct {
    game_state_t *state;
    game_sync_t *sync;

    size_t shm_total_size;
    entity_t who;
} game_t;

/**
 * @brief Parameters forwarded to @ref _new_game.
 *
 * Aggregated by the @ref new_game convenience macro, which automatically
 * fills @c manage_error and @c caller from the call site.
 */
typedef struct {
    error_manager_t manage_error;
    trace_t caller;
    uint64_t seed;
    uint16_t width;
    uint16_t height;
} game_params_t;

static const char *const default_view_path = NULL;
static const uint64_t default_width = 10;
static const char *const default_c_width = "10";
static const uint64_t default_heigh = 10;
static const char *const default_c_height = "10";
static const uint64_t default_delay = 200;
static const uint64_t default_timeout = 10;

/**
 * @brief Convenience macro to create a game handle with call-site tracing.
 *
 * Fills @c manage_error and @c caller automatically, then forwards the
 * remaining named-initialiser fields to @ref _new_game.
 *
 * @param who  Entity type (master / player / view).
 * @param ...  Optional field overrides for @ref game_params_t
 *             (e.g. @c .width = 20, .height = 20, .seed = 42).
 */
#define new_game(who, ...)                                                                                             \
    _new_game((who), (game_params_t){                                                                                  \
                         .manage_error = (manage_error),                                                               \
                         .caller = HERE,                                                                               \
                         .seed = (time(NULL)),                                                                         \
                         .width = (default_width),                                                                     \
                         .height = (default_heigh),                                                                    \
                         __VA_ARGS__ /* WARNING: trailing comma brakes this trick */                                   \
                     })

/**
 * @brief Create a game handle and map both shared memory segments.
 *
 * The master entity creates the segments and initialises the sync
 * semaphores. Players and the view attach to existing segments.
 *
 * @param who              Entity type.
 * @param game_parameters  Aggregated creation parameters.
 * @return                 Fully initialised game handle.
 */
game_t _new_game(entity_t who, game_params_t game_parameters);

/**
 * @brief Unmap shared memory and (if master) unlink the POSIX objects.
 *
 * @param game Game handle to disconnect.
 */
void game_disconnect(game_t *game);

/**
 * @brief Mark the game as finished and unblock all player semaphores.
 *
 * Sets @c state->running to false and posts every player turn semaphore
 * so that blocked players can detect the shutdown.
 *
 * @param game Game handle.
 */
void game_end(game_t *game);

/**
 * @brief Return the number of registered players.
 *
 * @param game Game handle.
 * @return     Player count.
 */
uint_fast8_t players_ingame(game_t *game);

/**
 * @brief Check whether a process with the given PID is a registered player.
 *
 * @param game      Game handle.
 * @param player_id PID to look up.
 * @return          true if found, false otherwise.
 */
bool is_player_ingame(game_t *game, pid_t player_id);
