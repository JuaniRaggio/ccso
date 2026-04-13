#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>

#include "game.h"
#include "game_admin.h"
#include "game_sync.h"
#include "master_loop.h"
#include "pipes.h"
#include <error_management.h>
#include <parser.h>

int main(int argc, char *argv[]) {
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

    bool interrupted = master_run(&game, &params, pipes, view_pid, &has_view);

    game_end(&game);
    if (has_view)
        game_sync_notify_view(game.sync);
    close_active_pipes(pipes, game.state->players, params.players_count);

    for (int8_t i = 0; i < params.players_count; i++) {
        if (waitpid(game.state->players[i].player_id, NULL, WNOHANG) == 0)
            waitpid(game.state->players[i].player_id, NULL, 0);
    }
    if (has_view) {
        if (waitpid(view_pid, NULL, WNOHANG) == 0)
            waitpid(view_pid, NULL, 0);
    }

    print_game_results(game.state);
    game_sync_destroy(game.sync);
    game_disconnect(&game);

    if (interrupted) {
        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }

    return 0;
}
