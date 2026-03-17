#include <stdio.h>
#include "../common/include/game_state.h"

// Create shared Memory with flags, permissions, etc
game_state_t * createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions, int mapFlag);

// Initialize the board (height X width) with random numbers from 1 to 9
void initializeBoard(int8_t board[], uint16_t height, uint16_t width);

// Returns a random number between 1 and 9
int random_1_9();
