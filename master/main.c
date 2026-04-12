#include <errno.h>
#include <fcntl.h>
#include <parser.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "game.h"
#include "game_admin.h"
#include "pipes.h"
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

    if (status != success || parameters.players_count == 0) {
        // TODO: improve error management for user parameter information
        return manage_error(HERE, TRACE_NONE, invalid_argument_error);
    }

    errno = 0;
    game_t game = new_game(master, .height = parameters.height, .width = parameters.width, .seed = parameters.seed);
    game_state_init(&game, parameters.width, parameters.height, parameters.seed, parameters.players_count);

    const bool has_view = parameters.view_path != NULL;
    if (has_view) {
        fork_view(parameters.view_path, parameters.c_width, parameters.c_height);
    }

    int pipes[MAX_PLAYERS][pipe_ends], players_count;
    players_count = game.state->players_count;

    create_pipes(pipes, players_count);
    fork_players(pipes, players_count, game.state);

    close_other_pipes(pipes, players_count, invalid_pipe, pipe_writer);

    /*
    IDEA: crear un masterSet y ahi cargar todos los filesDescriptors. Para el SELECT, usar el readFds, ya que el select
    lo cambia (deja unicamente los que tienen algo para leer), entonces en la proxima iteracion, lo unico que deberiamos
    hacer es readFds = masterSet
    */
    int maxFd;
    fd_set masterSet, readFds;
    init_fd_set(&masterSet, pipes, players_count, &maxFd);
}
