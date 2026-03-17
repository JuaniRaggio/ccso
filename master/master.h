#include <stdio.h>
#include "../common/include/game_state.h"

game_state_t * createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions, int mapFlag);
