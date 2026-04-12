#include <errno.h>
#include <fcntl.h>
#include <parser.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "game_admin.h"
#include "game_sync.h"
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
        .c_width = (char *)default_c_width,
        .height = default_heigh,
        .c_height = (char *)default_c_height,
        .delay = default_delay,
        .timeout = default_timeout,
        .seed = time(NULL),
        .view_path = default_view_path,
    };

    parameter_status_t status = parse(argc, argv, &parameters);
    if (status != success || parameters.players_count == 0) {
        fprintf(stderr, "Usage: master -w <width> -h <height> -p <player> [-p <player> ...] "
                        "[-v <view>] [-d <delay>] [-t <timeout>] [-s <seed>]\n");
        return manage_error(HERE, TRACE_NONE, invalid_argument_error);
    }

    errno = 0;
    game_t game = new_game(master, .height = parameters.height, .width = parameters.width, .seed = parameters.seed);
    game_state_init(&game, parameters.width, parameters.height, parameters.seed, parameters.players_count);

    int32_t pipes[MAX_PLAYERS][pipe_ends];
    int8_t players_count = game.state->players_count;

    create_pipes(pipes, players_count);

    bool has_view = parameters.view_path != NULL;
    pid_t view_pid = 0;
    if (has_view) {
        view_pid = fork_view(parameters.view_path, parameters.c_width, parameters.c_height);
    }

    fork_players(pipes, players_count, game.state, parameters.players_paths, parameters.c_width, parameters.c_height);
    close_other_pipes(pipes, players_count, invalid_pipe, pipe_writer);

    register_players_from_paths(game.state, parameters.players_paths);
    place_players_on_board(game.state);
    game.state->running = true;

    int32_t maxFd;
    fd_set masterSet, readFds;
    init_fd_set(&masterSet, pipes, players_count, &maxFd);

    time_t last_valid_move = time(NULL);
    int8_t start_player = 0;

    while (!should_exit) {
        time_t now = time(NULL);
        int64_t remaining = (int64_t)parameters.timeout - (int64_t)(now - last_valid_move);
        if (remaining <= 0)
            break;

        readFds = masterSet;
        struct timeval tv = {
            .tv_sec = remaining,
            .tv_usec = 0,
        };
        int32_t ready = select(maxFd + 1, &readFds, NULL, NULL, &tv);

        if (ready < 0) {
            if (errno == EINTR)
                continue;
            manage_error(HERE, TRACE_NONE, errno);
            break;
        }
        if (ready == 0)
            break;

        bool any_move = false;
        bool any_valid = false;
        int8_t last_processed = start_player;

        for (int8_t i = 0; i < players_count; i++) {
            int8_t idx = (start_player + i) % players_count;
            if (handle_player_turn(&game, pipes, &readFds, &masterSet, idx, &any_valid)) {
                any_move = true;
                last_processed = idx;
            }
        }

        start_player = (last_processed + 1) % players_count;

        if (any_valid)
            last_valid_move = time(NULL);

        if (any_move && has_view) {
            if (waitpid(view_pid, NULL, WNOHANG) != 0) {
                has_view = false;
            } else {
                game_sync_view_cycle(game.sync);
            }
        }

        if (any_move)
            usleep(parameters.delay * 1000);

        if (!any_player_alive(game.state))
            break;
    }

    game_end(&game);

    if (has_view)
        game_sync_notify_view(game.sync);

    close_active_pipes(pipes, game.state->players, players_count);

    for (int8_t i = 0; i < players_count; i++) {
        waitpid(game.state->players[i].player_id, NULL, 0);
    }
    if (has_view)
        waitpid(view_pid, NULL, 0);

    print_game_results(game.state);

    game_sync_destroy(game.sync);
    game_disconnect(&game);

    return 0;
}
