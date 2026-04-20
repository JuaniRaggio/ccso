#include "master_loop.h"
#include "game_admin.h"
#include "game_sync.h"
#include "pipes.h"
#include <errno.h>
#include <signals.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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

bool master_run(game_t *game, parameters_t *params, int32_t pipes[][pipe_ends], pid_t view_pid, bool *has_view) {
    setup_signals();

    int32_t maxFd;
    fd_set masterSet;
    init_fd_set(&masterSet, pipes, params->players_count, &maxFd);

    time_t last_valid_move = time(NULL);
    int8_t start_player = 0;

    while (!was_interrupted() && any_player_alive(game->state)) {
        if (check_timeout(last_valid_move, params->timeout))
            break;

        fd_set readFds = masterSet;
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

        if (*has_view) {
            sync_view_frame(game, view_pid, has_view);
        }

        round_result_t round = process_round(game, pipes, &readFds, &masterSet, start_player);
        start_player = round.next_start_player;

        if (round.any_valid)
            last_valid_move = time(NULL);

        if (!any_player_alive(game->state))
            break;

        if (round.any_move)
            usleep(params->delay * 1000);
    }

    return was_interrupted();
}
