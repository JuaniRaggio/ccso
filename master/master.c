#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "../common/include/game_state.h"
#include "../common/include/game_sync.h"
#include "master.h"

#define PERMISSIONS 0666
#define MAPFLAG MAP_SHARED
#define PROTECTIONS PROT_READ | PROT_WRITE

//#define OPENFLAGS O_CREAT | O_RDWR Generates Confusission -> The master use both of them, the other processes only use O_RDWR (ftruncate)

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

int main(int argc, char* argv[]){

   /*
   We need a function that Store the parameters that the user entered via STDIN. 
   First we save them (localVars or struct), then  we create sharedMemory, after that we save them in the sharedMemory struct
   */
   int height = 3, width = 24, players_count = 8; // This should not be here ( We need a function to get them from stdin)



   // We create the sharedMemory for "game state" and assign a pointer to its beginning
   game_state_t * sharedGameState = NULL;
   int totalSize = (sizeof(game_state_t) + (sizeof(int8_t) * height * width));
   sharedGameState = createSharedMemory("/game_state", totalSize, O_CREAT | O_RDWR, PERMISSIONS, PROTECTIONS, MAPFLAG);
   // When we asign the sharedMemory to a struct pointer, the sharedMemory beahaves like a the struct (sharedGameState->...)

   // We create the sharedMemory for "game sync" and assign a pointer to its beginning
   game_sync_t * sharedGameSync = NULL;
   sharedGameSync = createSharedMemory("/game_sync", sizeof(game_sync_t), O_CREAT | O_RDWR, PERMISSIONS, PROTECTIONS, MAPFLAG);
   
   initalizeGameState(sharedGameState, width, height, players_count);
   
   printGameState(sharedGameState->board, sharedGameState->height, sharedGameState->width, sharedGameState->players_count, sharedGameState->state);

   
   
   // We initalize and print the sem A value, to check if the sharedMemory was created propierly and the value was saved correctly
   sem_init(&sharedGameSync->A, 1, 231);
   sem_post(&sharedGameSync->A); // Increase the sem (232)
   int value;
   sem_getvalue(&sharedGameSync->A, &value);
   printf("Valor del semáforo: %d\n", value);
   
}


/*
totalSize:
   -> In the shared memory: "/game_state", totalSize = sizeOf(game_state_t) + boardSize
   -> In the shared memory: "/game_sync", totalSize = sizeOf(struct ZZZ)
*/
void * createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions, int mapFlag){

   // Use shm_open -> create the shared Memory with size 0
   int fd = shm_open(sharedMemoryName, openFlags, permissions);
   if(fd == -1){
      perror("shm_open");
      return NULL;
   }
   // Use ftruncate -> assign size to the shared Memory 
   if(openFlags & O_CREAT){   // Only the process that creates the sharedMemory (master) modify its size. 
      if( ftruncate(fd, totalSize) == -1){
         perror("ftruncate");
         close(fd);
         return NULL;
      }
   }
   // Use mmap -> Conect the shared Memory with a pointer from our program
   void * ptr = mmap(NULL, totalSize, proteccions, mapFlag, fd, 0); 
   if (ptr == MAP_FAILED) {
      perror("mmap");
      close(fd);
      return NULL;
   }
   close(fd);
   return ptr;
}

// void initalizeGameState(game_state_t * sharedGameState, uint16_t width, uint16_t height, int8_t players_count, player_t players[9])
void initalizeGameState(game_state_t * sharedGameState, uint16_t width, uint16_t height, int8_t players_count){
   
   sharedGameState->width = width;
   sharedGameState->height = height;
   sharedGameState->players_count = players_count;
   sharedGameState->state = 0;
   initializeBoard(sharedGameState->board, height, width);
   return;
} // We NEED to complete the initialization of the player_t

// We initialize the board with random numbers between 1 and 9
void initializeBoard(int8_t board[], uint16_t height, uint16_t width){
   
   srand(time(NULL));
   for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
         board[ 4*i + j ] = random_1_9();
      }
   }
   return;
}

int random_1_9() {
   return (rand() % 9) + 1;
}


// -----------------------------------------------------------------------------------------------------------------

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state){

   printf("WIDTH: %d\nHEIGHT: %d\nPLAYER COUNT: %d\nSTATE: %d\n\nBOARD:\n", width, height, players_count, state);
   printBoard(board, height, width);
}

void printBoard(int8_t board[], uint16_t height, uint16_t width){
   for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
         printf("%d ",board[ 4*i + j] ); //De izq a der y de arriba a abajo (en ese orden)
      }
      printf("\n");
   }
   printf("\n");
   return;
}

// ------------------------------------------------------------------------------------------------------------------
