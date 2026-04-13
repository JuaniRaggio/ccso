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

static void setup_signals() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

static void sync_view_frame(game_t *game, pid_t view_pid, bool *has_view) {
    if (waitpid(view_pid, NULL, WNOHANG) != 0) {
        *has_view = false;
    } else {
        game_sync_view_cycle(game->sync);
    }
}

static bool check_timeout(time_t last_valid_move, uint32_t timeout) {
    return (uint32_t)(time(NULL) - last_valid_move) >= timeout;
}

static void run_master_loop(game_t *game, parameters_t *params, int32_t pipes[][pipe_ends], fd_set *masterSet,
                            int32_t maxFd, pid_t view_pid, bool *has_view) {
    time_t last_valid_move = time(NULL);
    int8_t start_player = 0;

    while (!should_exit && any_player_alive(game->state)) {
        if (check_timeout(last_valid_move, params->timeout))
            break;

        fd_set readFds = *masterSet;
        time_t remaining = (time_t)params->timeout - (time(NULL) - last_valid_move);
        if (remaining <= 0)
            break;
        struct timeval tv = {
            .tv_sec = remaining > 1 ? 1 : remaining,
            .tv_usec = 0,
        };

        int sel = select(maxFd + 1, &readFds, NULL, NULL, &tv);
        if (sel < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (sel == 0)
            continue;

        round_result_t round = process_round(game, pipes, &readFds, masterSet, start_player);
        start_player = round.next_start_player;

        if (round.any_valid)
            last_valid_move = time(NULL);

        if (*has_view) {
            sync_view_frame(game, view_pid, has_view);
        }

        if (!any_player_alive(game->state))
            break;

        if (round.any_move)
            usleep(params->delay * 1000);
    }
}

int main(int argc, char *argv[]) {
    setup_signals();
    parameters_t params = {.width = default_width,
                           .height = default_heigh,
                           .delay = default_delay,
                           .timeout = default_timeout,
                           .seed = time(NULL),
                           .view_path = default_view_path};

    if (parse(argc, argv, &params) != success || params.players_count == 0) {
        fprintf(stderr, "Usage: master -w <width> -h <height> -p <player> [-p <player> ...] [-v <view>] [-d <delay>] "
                        "[-t <timeout>] [-s <seed>]\n");
        return manage_error(HERE, TRACE_NONE, invalid_argument_error);
    }

    game_t game = new_game(master, .height = params.height, .width = params.width, .seed = params.seed);
    game_state_init(&game, params.width, params.height, params.seed, params.players_count);

    int32_t pipes[MAX_PLAYERS][pipe_ends];
    create_pipes(pipes, params.players_count);

    bool has_view = params.view_path != NULL;
    pid_t view_pid = has_view ? fork_view(params.view_path, params.c_width, params.c_height) : 0;

    fork_players(pipes, params.players_count, game.state, params.players_paths, params.c_width, params.c_height);
    close_other_pipes(pipes, params.players_count, invalid_pipe, pipe_writer);

    register_players_from_paths(game.state, params.players_paths);
    place_players_on_board(game.state);
    game.state->running = true;

    int32_t maxFd;
    fd_set masterSet;
    init_fd_set(&masterSet, pipes, params.players_count, &maxFd);

    run_master_loop(&game, &params, pipes, &masterSet, maxFd, view_pid, &has_view);

    game_end(&game);
    if (has_view)
        game_sync_notify_view(game.sync);
    close_active_pipes(pipes, game.state->players, params.players_count);

    for (int8_t i = 0; i < params.players_count; i++)
        waitpid(game.state->players[i].player_id, NULL, 0);
    if (has_view)
        waitpid(view_pid, NULL, 0);

    print_game_results(game.state);
    game_sync_destroy(game.sync);
    game_disconnect(&game);

    return 0;
}
