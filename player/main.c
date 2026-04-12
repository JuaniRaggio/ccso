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
#include <shmemory_utils.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
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

    game_disconnect(&game);
    return 0;
}
