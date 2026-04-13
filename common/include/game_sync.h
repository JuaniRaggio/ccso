/**
 * @file game_sync.h
 * @brief Inter-process synchronisation primitives for the game.
 *
 * Implements a readers-writer lock (with writer-preference turnstile),
 * a view render/done handshake, and per-player turn semaphores. All
 * semaphores live in shared memory and are initialised with
 * @c pshared = 1 so they work across forked processes.
 */
#pragma once

#include "game_state.h"
#include <semaphore.h>
#include <stdint.h>

/** @brief pshared value for sem_init: shared between processes. */
static const int32_t SEM_SHARED_BETWEEN_PROCESSES = 1;

/** @brief Initial semaphore value: blocked (0). */
static const uint32_t SEM_LOCKED = 0;

/** @brief Initial semaphore value: available (1). */
static const uint32_t SEM_UNLOCKED = 1;

/** @brief POSIX shared memory object name for the sync segment. */
extern const char game_sync_memory_name[];

/**
 * @brief Synchronisation state stored in shared memory.
 *
 * Contains all semaphores needed for the readers-writer protocol,
 * the view handshake, and the per-player turn control.
 */
typedef struct {
    sem_t view_may_render;                       /**< Signalled when the view may start rendering. */
    sem_t view_rendered;                         /**< Signalled when the view has finished rendering. */
    sem_t master_writing;                        /**< Writer-preference turnstile semaphore. */
    sem_t gamestate_mutex;                       /**< Protects the game state from concurrent writes. */
    sem_t readers_count_mutex;                   /**< Protects the @c readers_count counter. */
    uint32_t readers_count;                      /**< Number of active readers. */
    sem_t player_may_send_movement[MAX_PLAYERS]; /**< Per-player turn semaphores. */
} game_sync_t;

/**
 * @brief Acquire exclusive write access to the game state.
 *
 * Blocks until no readers or other writers hold the lock.
 * Must be paired with @ref game_sync_writer_exit.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_writer_enter(game_sync_t *sync);

/**
 * @brief Release exclusive write access to the game state.
 *
 * Semaphores are posted in reverse acquisition order.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_writer_exit(game_sync_t *sync);

/**
 * @brief Acquire shared read access to the game state.
 *
 * Multiple readers may hold the lock concurrently. The first reader
 * acquires @c gamestate_mutex; subsequent readers only bump the counter.
 * Must be paired with @ref game_sync_reader_exit.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_reader_enter(game_sync_t *sync);

/**
 * @brief Release shared read access to the game state.
 *
 * The last reader releases @c gamestate_mutex, allowing writers to proceed.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_reader_exit(game_sync_t *sync);

/**
 * @brief Signal the view process that a new frame is available.
 *
 * Posts @c view_may_render so the view unblocks from
 * @ref game_sync_view_wait_frame.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_notify_view(game_sync_t *sync);

/**
 * @brief Block until the view process finishes rendering.
 *
 * Waits on @c view_rendered.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_wait_view_done(game_sync_t *sync);

/**
 * @brief Notify the view and wait for it to finish (combined cycle).
 *
 * Equivalent to calling @ref game_sync_notify_view followed by
 * @ref game_sync_wait_view_done.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_view_cycle(game_sync_t *sync);

/**
 * @brief Block until the master signals that a frame is ready.
 *
 * Called by the view process at the top of its render loop.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_view_wait_frame(game_sync_t *sync);

/**
 * @brief Signal the master that the view has finished rendering.
 *
 * Called by the view process after drawing a frame.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_view_frame_done(game_sync_t *sync);

/**
 * @brief Block a player until the master grants its turn.
 *
 * @param sync       Pointer to the shared sync structure.
 * @param player_idx Zero-based index of the player.
 */
void game_sync_player_wait_turn(game_sync_t *sync, uint8_t player_idx);

/**
 * @brief Grant a player permission to send its next move.
 *
 * @param sync       Pointer to the shared sync structure.
 * @param player_idx Zero-based index of the player.
 */
void game_sync_player_grant_turn(game_sync_t *sync, uint8_t player_idx);

/**
 * @brief Initialise all semaphores in the sync structure.
 *
 * Called once by the master process after mapping the shared memory.
 * Player turn semaphores start unlocked (SEM_UNLOCKED = 1) so each
 * player can immediately send its first move.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_init(game_sync_t *sync);

/**
 * @brief Destroy all semaphores in the sync structure.
 *
 * Called by the master process during cleanup.
 *
 * @param sync Pointer to the shared sync structure.
 */
void game_sync_destroy(game_sync_t *sync);
