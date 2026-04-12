#pragma once

/**
 * @brief: This is the sync's file for this project, it is placed in
 *         common/ just because the contract has to be simetric such
 *         as in any communication protocol
 **/

#include "game_state.h"
#include <semaphore.h>
#include <stdint.h>

static const int32_t SEM_SHARED_BETWEEN_PROCESSES = 1;
static const uint32_t SEM_LOCKED = 0;
static const uint32_t SEM_UNLOCKED = 1;

extern const char game_sync_memory_name[];

typedef struct {
    sem_t view_may_render;
    sem_t view_rendered;
    sem_t master_writing;
    sem_t gamestate_mutex;
    sem_t readers_count_mutex;
    uint32_t readers_count;
    sem_t player_may_send_movement[MAX_PLAYERS];
} game_sync_t;

void game_sync_writer_enter(game_sync_t *sync);
void game_sync_writer_exit(game_sync_t *sync);

void game_sync_reader_enter(game_sync_t *sync);
void game_sync_reader_exit(game_sync_t *sync);

void game_sync_notify_view(game_sync_t *sync);

void game_sync_wait_view_done(game_sync_t *sync);

void game_sync_view_cycle(game_sync_t *sync);

void game_sync_view_wait_frame(game_sync_t *sync);

void game_sync_view_frame_done(game_sync_t *sync);

void game_sync_player_wait_turn(game_sync_t *sync, uint8_t player_idx);
void game_sync_player_grant_turn(game_sync_t *sync, uint8_t player_idx);

void game_sync_init(game_sync_t *sync);
void game_sync_destroy(game_sync_t *sync);
