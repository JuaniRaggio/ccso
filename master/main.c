#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <parser.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "game.h"
#include "game_admin.h"
#include "game_state.h"
#include <error_management.h>

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    errno = 0;
    parameters_t parameters = (parameters_t){
        .width = default_width,
        .height = default_heigh,
        .delay = default_delay,
        .timeout = default_timeout,
        .seed = time(NULL),
        .view_path = default_view_path,
    };

    parameter_status_t status = parse(argc, argv, &parameters);

    if (status != success) {
        // Manage errors using status
    }

    errno = 0;
    game_t game = new_game(master, .height = parameters.height, .width = parameters.width, .seed = parameters.seed);
    game_state_init(&game, parameters.width, parameters.height, parameters.seed, parameters.players_count);

    player_t players[MAX_PLAYERS] = {};
    const bool has_view = parameters.view_path != NULL;
}
