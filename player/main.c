#include "game.h"
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <argv_parser.h>
#include <error_management.h>
#include <game_state.h>
#include <game_sync.h>
#include <player_movement.h>
#include <player_protocol.h>
#include <shmemory_utils.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

static int8_t find_player_index(game_state_t *state) {
    pid_t my_pid = getpid();
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].player_id == my_pid) {
            return i;
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    if (argc < 3) {
        fprintf(stderr, "Use: %s <width> <height>\n", argv[0]);
        return 1;
    }

    uint16_t width, height;
    if (!parse_board_args(argv, &width, &height)) {
        return manage_error(TRACE_NONE, HERE, invalid_argument_error);
    }

    game_t game = new_game(player, .height = height, .width = width);

    // TO DO: Player Loop
    int8_t my_idx = find_player_index(game.state);
    if (my_idx < 0) {
        game_disconnect(&game);
        return 1;
    }

    while (!should_exit && game.state->running) {
        game_sync_player_wait_turn(game.sync, (uint8_t)my_idx);
        if (should_exit || !game.state->running) {
            break;
        }
        game_sync_reader_exit(game.sync);
        send_direction(STDOUT_FILENO, dir);
    }

    game_disconnect(&game);
    return 0;
}
