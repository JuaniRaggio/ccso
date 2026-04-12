#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/select.h>
#include "game_state.h"

#define READ 0
#define WRITE 1

void createPipes(int pipes[][2], int playersCount);

void forkPlayers(int pipes[][2], int playersCount, game_state_t *game_state);

void closePipes(int pipes[][2], int action, int playersCount);

void initFdSet(fd_set *masterSet, int pipes[][2], int playersCount, int *maxFd);