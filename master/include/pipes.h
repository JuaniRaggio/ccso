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

void create_pipes(int pipes[][pipe_ends], int playersCount);

void fork_players(int pipes[][pipe_ends], int playersCount, game_state_t *game_state);

void close_pipes(int pipes[][pipe_ends], pipe_users_t action, int playersCount);

void init_fd_set(fd_set *masterSet, int pipes[][pipe_ends], int playersCount, int *maxFd);
