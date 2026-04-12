#include <pipes.h>
#include <errno.h>
#include <error_management.h>
#include <stdint.h>
#include <stdlib.h>

void create_pipes(int pipes[][pipe_ends], int playersCount) {
    for (int i = 0; i < playersCount; i++) {
        if (pipe(pipes[i]) == -1) {
            manage_error(HERE, TRACE_NONE, errno);
            exit(EXIT_FAILURE);
        }
    }
}

static void close_other_pipes(int32_t pipes[][pipe_ends], uint32_t player_count, uint32_t dont_close_this_player,
                              const pipe_users_t user_to_close) {
    // const here is an optimization for the compiler,
    // since it doesn't change inside this function, the comparison shouldn't be
    // done for each iteration
    for (uint32_t player_index = 0; player_index < player_count; ++player_index) {
        if (dont_close_this_player != player_index) {
            close(pipes[player_index][pipe_writer]);
        }
    }
}

void close_pipes(int pipes[][pipe_ends], pipe_users_t action, int playersCount) {
    for (int i = 0; i < playersCount; i++) {
        close(pipes[i][action]);
    }
}

void fork_players(int pipes[][pipe_ends], int playersCount, game_state_t *game_state) {

    for (int i = 0; i < playersCount; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            manage_error(HERE, TRACE_NONE, errno);
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {

            // son should not read => write only => close read-end
            close_pipes(pipes, pipe_reader, playersCount);
            close_other_pipes(pipes, playersCount, i, pipe_writer);

            // Redirigir stdout -> pipe
            // dup2 cierra el FD "stdout" y lo abre con el write-end del pipe
            // Redirige la salida del proceso al pipe (EJ: Si hago un printf(), se imprime en el write-end del pipe)
            if (dup2(pipes[i][pipe_writer], STDOUT_FILENO) == -1) {
                manage_error(HERE, TRACE_NONE, errno);
                _exit(EXIT_FAILURE);
            }

            // Cierro el FD original (ya esta duplicado gracias a dup2)
            close(pipes[i][pipe_writer]);

            // Ejecutamos el programa player[i].name
            char *args[] = {game_state->players[i].name, NULL};
            sleep(3);
            execve(game_state->players[i].name, args, NULL);

            // Si llego aca es porque execve fallo. Ya que sino execve no retorna
            manage_error(HERE, TRACE_NONE, errno);
            _exit(EXIT_FAILURE);
        }

        game_state->players[i].player_id = pid;
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
