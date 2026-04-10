#include "game.h"
#include <semaphore.h>
#include <stdint.h>
#include <unistd.h>

#include <game_state.h>
#include <game_sync.h>
#include <player_movement.h>

// ocupamos lectura
void start_reading_board(game_sync_t *sync) {
    sem_wait(sync->master_writing);
    sem_post(sync->master_writing);

    sem_wait(sync->readers_count_mutex);
    if (sync->readers_count == 0) {
        sync->readers_count++;
        sem_wait(sync->gamestate_mutex);
    }
    sem_post(sync->readers_count_mutex);
    return;
}

// liberamos lectura
void stop_reading_bord(game_sync_t *sync) {
    sem_wait(sync->readers_count_mutex);
    sync->readers_count--;
    if (sync->readers_count == 0) {
        sem_post(sync->gamestate_mutex);
    }
    sem_post(sync->readers_count_mutex);
    return;
}

void start_playing(game_t *game, uint16_t idx) {
    while (game->state->running) {
        // Waiting for the master to allow reading
        sem_wait(game->sync->player_may_send_movement[idx]);
        if (!game->state->running)
            break;

        start_reading_board(game->sync);
        if (!game->state->running)
            break;

        int8_t move = compute_next_move(game->state, idx);

        stop_reading_board(game->sync);
        if (!game->state->running)
            break;

        write(1, &move, 1);
    }
    return;
}