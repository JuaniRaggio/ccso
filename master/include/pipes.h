#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/select.h>
#include "game_state.h"


typedef enum {
    pipe_reader = 0,
    pipe_writer,
    pipe_ends,
} pipe_users_t;

void createPipes(int pipes[][pipe_ends], int playersCount);

void forkPlayers(int pipes[][pipe_ends], int playersCount, game_state_t *game_state);

void closePipes(int pipes[][pipe_ends], int action, int playersCount);

void initFdSet(fd_set *masterSet, int pipes[][pipe_ends], int playersCount, int *maxFd);
