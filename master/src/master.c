#include <stdint.h>
#include <stdio.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/semaphore.h>
#include <time.h>
#include <unistd.h>

#include <game_state.h>
#include <game_sync.h>
#include "master.h"
/*
totalSize:
   -> In the shared memory: "/game_state", totalSize = sizeOf(game_state_t) +
boardSize
   -> In the shared memory: "/game_sync", totalSize = sizeOf(struct ZZZ)
*/
void *createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions,
                         int mapFlag) {

   // Use shm_open -> create the shared Memory with size 0
   int fd = shm_open(sharedMemoryName, openFlags, permissions);
   if (fd == -1) {
      perror("shm_open");
      return NULL;
   }
   // Use ftruncate -> assign size to the shared Memory
   if (openFlags & O_CREAT) { // Only the process that creates the sharedMemory
                              // (master) modify its size.
      if (ftruncate(fd, totalSize) == -1) {
         perror("ftruncate");
         close(fd);
         return NULL;
      }
   }
   // Use mmap -> Conect the shared Memory with a pointer from our program
   void *ptr = mmap(NULL, totalSize, proteccions, mapFlag, fd, 0);
   if (ptr == MAP_FAILED) {
      perror("mmap");
      close(fd);
      return NULL;
   }
   close(fd);
   return ptr;
}

// void initalizeGameState(game_state_t * sharedGameState, uint16_t width,
// uint16_t height, int8_t players_count, player_t players[9])
void initalizeGameState(game_state_t *sharedGameState, uint16_t width, uint16_t height, int8_t players_count) {

   sharedGameState->width = width;
   sharedGameState->height = height;
   sharedGameState->players_count = players_count;
   sharedGameState->state = 0;
   initializeBoard(sharedGameState->board, height, width);
   return;
} // We NEED to complete the initialization of the player_t

// We initialize the board with random numbers between 1 and 9
void initializeBoard(int8_t board[], uint16_t height, uint16_t width) {

   srand(time(NULL));
   for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
         board[4 * i + j] = random_1_9();
      }
   }
   return;
}

int random_1_9() {
   return (rand() % 9) + 1;
}

// -----------------------------------------------------------------------------------------------------------------

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state) {

   printf("WIDTH: %d\nHEIGHT: %d\nPLAYER COUNT: %d\nSTATE: %d\n\nBOARD:\n", width, height, players_count, state);
   printBoard(board, height, width);
}

void printBoard(int8_t board[], uint16_t height, uint16_t width) {
   for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
         printf("%d ",
                board[4 * i + j]); // De izq a der y de arriba a abajo (en ese orden)
      }
      printf("\n");
   }
   printf("\n");
   return;
}

// ------------------------------------------------------------------------------------------------------------------
