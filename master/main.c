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
        return manage_error(HERE, TRACE_NONE, invalid_argument_error);
    }

    errno = 0;
    game_t game = new_game(master, .height = parameters.height, .width = parameters.width, .seed = parameters.seed);
    game_state_init(&game, parameters.width, parameters.height, parameters.seed, parameters.players_count);

    int32_t pipes[MAX_PLAYERS][pipe_ends];
    int8_t players_count = game.state->players_count;

    create_pipes(pipes, players_count);

    const bool has_view = parameters.view_path != NULL;
    pid_t view_pid = 0;
    if (has_view) {
        view_pid = fork_view(parameters.view_path, parameters.c_width, parameters.c_height);
    }

    fork_players(pipes, players_count, game.state, parameters.players_paths, parameters.c_width, parameters.c_height);

    close_other_pipes(pipes, players_count, invalid_pipe, pipe_writer);

    for (int8_t i = 0; i < players_count; i++) {
        const char *base = strrchr(parameters.players_paths[i], '/');
        base = base ? base + 1 : parameters.players_paths[i];
        player_registration_requirements_t req = {
            .player_pid = game.state->players[i].player_id,
        };
        strncpy((char *)req.name, base, MAX_NAME_LENGTH - 1);
        ((char *)req.name)[MAX_NAME_LENGTH - 1] = '\0';
        game_register_player(game.state->players, i, req);
    }
    place_players_on_board(game.state);
    game.state->running = true;

    int32_t maxFd;
    fd_set masterSet, readFds;
    init_fd_set(&masterSet, pipes, players_count, &maxFd);
    time_t last_valid_move = time(NULL);
    int8_t start_player = 0;
    while (!should_exit) {
    }
    game_sync_destroy(game.sync);
    game_disconnect(&game);

    return 0;
}
