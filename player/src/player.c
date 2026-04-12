#include "game.h"
#include <semaphore.h>
#include <stdint.h>
#include <unistd.h>

#include <game_state.h>
#include <game_sync.h>
#include <player_movement.h>

/*
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
*/