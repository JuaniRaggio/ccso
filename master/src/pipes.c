#include "pipes.h"

// Creamos los pipes necesarios para la # de players
void createPipes(int pipes[][2], int playersCount) {
    for (int i = 0; i < playersCount; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
} //[read, write]

void forkPlayers(int pipes[][2], int playersCount, game_state_t *game_state) {

    for (int i = 0; i < playersCount; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) { // HIJO i

            // Proceso Hijo (player) no debe leer del pipe -> Cerramos todos los read-end del hijo (de cada hijo)
            closePipes(pipes, READ, playersCount);

            // Cada hijo debe escribir unicamente en un pipe -> Cerramos todos los demas write-end del hijo (de cada
            // hijo)
            for (int j = 0; j < playersCount; j++) {
                if (j != i) {
                    close(pipes[j][WRITE]);
                }
            }

            // Redirigir stdout → pipe
            // dup2 cierra el FD "stdout" y lo abre con el write-end del pipe
            // Redirige la salida del proceso al pipe (EJ: Si hago un printf(), se imprime en el write-end del pipe)
            dup2(pipes[i][WRITE], STDOUT_FILENO);

            // Cierro el FD original (ya está duplicado gracias a dup2)
            close(pipes[i][WRITE]);

            // Ejecutamos el programa player[i].name
            char *args[] = {game_state->players[i].name, NULL};
            sleep(3);
            execve(game_state->players[i].name, args, NULL);

            // Si llego aca es porque execve fallo. Ya que sino execve no retorna
            perror("execve");
            exit(1);
        }

        game_state->players[i].player_id = pid;
    }
}

void closePipes(int pipes[][2], int action, int playersCount) {
    for (int i = 0; i < playersCount; i++) {
        close(pipes[i][action]);
    }
}

void initFdSet(fd_set *masterSet, int pipes[][2], int playersCount, int *maxFd) {

    FD_ZERO(masterSet);
    *maxFd = 0;

    // Agrego los (read-end, de los pipes) al set. (Agrega el numero de fd)
    for (int i = 0; i < playersCount; i++) {
        FD_SET(pipes[i][READ], masterSet); // Agrega el READ-END de todos los pipes a readfds (por eso necesita
                                           // &readfds)

        if (pipes[i][READ] > *maxFd) {
            *maxFd =
                pipes[i]
                     [READ]; // Si el fd del pipe actual es mayor que maxfd, actualizar. Asi nos quedamos con el mayor
        }
    }
}