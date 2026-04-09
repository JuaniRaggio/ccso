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
#include "game_state.h"
#include "master.h"
#include "shmemory_utils.h"
#include <error_management.h>

// This should be on a ADT
static const uint64_t default_width = 10;
static const uint64_t default_heigh = 10;
static const uint64_t default_delay = 200;
static const uint64_t default_timeout = 10;
static const char *const default_view_path = "";

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

int main(int argc, char *argv[]) {
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

    // TODO: usar tads para los sharedGameState y sync y unificar la inicializacion de parameters con la creacion de
    // memoria

    errno = 0;
    size_t totalSize = (sizeof(game_state_t) + sizeof(int8_t) * parameters.height * parameters.width);

    game_t game = new_game(master);

    player_t players[MAX_PLAYERS] = {};
    initalizeGameState(sharedGameState, parameters.width, parameters.height, parameters.players_count, players);

    printGameState(sharedGameState->board, sharedGameState->height, sharedGameState->width,
                   sharedGameState->players_count, sharedGameState->state);
}
