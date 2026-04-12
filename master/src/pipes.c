#include "game_state.h"
#include <pipes.h>
#include <errno.h>
#include <error_management.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/_types/_pid_t.h>
#include <unistd.h>

void create_pipes(int pipes[][pipe_ends], int playersCount) {
    for (int i = 0; i < playersCount; i++) {
        if (pipe(pipes[i]) == -1) {
            manage_error(HERE, TRACE_NONE, errno);
            exit(EXIT_FAILURE);
        }
    }
}

void close_other_pipes(int32_t pipes[][pipe_ends], uint32_t pipe_count, ssize_t dont_close_this_pipe,
                       const pipe_users_t user_to_close) {
    // const here is an optimization for the compiler,
    // since it doesn't change inside this function, the comparison shouldn't be
    // done for each iteration
    for (ssize_t pipe_index = 0; pipe_index < pipe_count; ++pipe_index) {
        if (dont_close_this_pipe != pipe_index) {
            close(pipes[pipe_index][user_to_close]);
        }
    }
}

static pid_t new_process() {
    pid_t pid = fork();
    if (pid < 0) {
        manage_error(HERE, TRACE_NONE, errno);
        exit(EXIT_FAILURE);
    }
    return pid;
}

pid_t fork_view(char * view_path, char * width, char * height) {
    if (view_path == NULL) {
        manage_error(HERE, TRACE_NONE, invalid_argument_error);
        exit(EXIT_FAILURE);
    }
    pid_t view_pid = new_process();
    if (view_pid == 0) {
        char *args[] = {
            [0] = view_path,
            [1] = width,
            [2] = height,
        };
        execve(view_path, args, NULL);
        manage_error(HERE, TRACE_NONE, unreachable);
        _exit(EXIT_FAILURE);
    }
    return view_pid;
}

void fork_players(int pipes[][pipe_ends], int playersCount, game_state_t *game_state) {
    for (int i = 0; i < playersCount; i++) {
        pid_t new_player = new_process();
        if (new_player == 0) {

            // son should not read => write only => close read-end
            close_other_pipes(pipes, playersCount, invalid_pipe, pipe_reader);
            close_other_pipes(pipes, playersCount, i, pipe_writer);

            if (dup2(pipes[i][pipe_writer], STDOUT_FILENO) == -1) {
                manage_error(HERE, TRACE_NONE, errno);
                _exit(EXIT_FAILURE);
            }

            close(pipes[i][pipe_writer]);

            char *args[] = {game_state->players[i].name, NULL};
            execve(game_state->players[i].name, args, NULL);
            manage_error(HERE, TRACE_NONE, unreachable);
            _exit(EXIT_FAILURE);
        }
        game_state->players[i].player_id = new_player;
    }
}

void init_fd_set(fd_set *masterSet, int pipes[][pipe_ends], int playersCount, int *maxFd) {

    FD_ZERO(masterSet);
    *maxFd = 0;

    // Agrego los (read-end, de los pipes) al set. (Agrega el numero de fd)
    for (int i = 0; i < playersCount; i++) {
        FD_SET(pipes[i][pipe_reader], masterSet); // Agrega el pipe_reader-END de todos los pipes a readfds (por eso
                                                  // necesita &readfds)

        if (pipes[i][pipe_reader] > *maxFd) {
            *maxFd = pipes[i][pipe_reader];
        }
    }
}
