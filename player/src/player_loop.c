#include <game.h>
#include <game_state.h>
#include <game_sync.h>
#include <player_movement.h>
#include <player_protocol.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

static int8_t find_player_index(game_state_t *state) {
    pid_t my_pid = getpid();
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].player_id == my_pid)
            return i;
    }
    return -1;
}

void player_run(game_t *game) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int8_t my_idx = find_player_index(game->state);
    if (my_idx < 0)
        return;

    game_sync_player_wait_turn(game->sync, (uint8_t)my_idx);

    while (!should_exit) {
        game_sync_reader_enter(game->sync);
        direction_wire_t dir = compute_next_move(game->state->board, game->state->width, game->state->height,
                                                 game->state->players[my_idx].x, game->state->players[my_idx].y);
        game_sync_reader_exit(game->sync);

        if (dir == NO_VALID_MOVE)
            break;

        send_direction(STDOUT_FILENO, dir);
        game_sync_player_wait_turn(game->sync, (uint8_t)my_idx);
        if (!game->state->running)
            break;
    }
}
