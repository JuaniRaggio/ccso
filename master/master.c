#include <stdio.h>
#include <time.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "../common/include/game_state.h"
#include "master.h"

#define PERMISSIONS 0666
#define MAPFLAG MAP_SHARED
#define OPENFLAGS O_CREAT | O_RDWR
#define PROTECCTIONS PROT_READ | PROT_WRITE

void printBoard(int8_t board[], uint16_t height, uint16_t width);

int main(int argc, char* argv[]){

   /*
   We need a function that Store the parameters that the user entered via STDIN. 
   First we save them (localVars or struct), then  we create sharedMemory, after that we save them in the sharedMemory struct
   */

   int height = 3, width = 4; // This should not be here
   game_state_t * sharedMemory = NULL;
   sharedMemory = createSharedMemory("/game_state", (sizeof(game_state_t) + (sizeof(int8_t) * height *width)), OPENFLAGS, PERMISSIONS, PROTECCTIONS, MAPFLAG);
   // Con ese puntero accedemos a la memoria compartida dsd cualquier proceso posterior al fork()

   initializeBoard(sharedMemory->board, height, width);
   printBoard(sharedMemory->board, height, width); //Para ver que se guardo el board en la shared memory
}


/*
totalSize:
   -> In the shared memory: "/game_state", totalSize = sizeOf(game_state_t) + boardSize
   -> In the shared memory: "/game_sync", totalSize = sizeOf(struct ZZZ)
*/
game_state_t * createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions, int mapFlag){

   // Use shm_open -> create the shared Memory with size 0
   int fd = shm_open(sharedMemoryName, openFlags, permissions);
   if(fd == -1){
      perror("shm_open");
      return NULL;
   }
   // Use ftruncate -> assign size to the shared Memory 
   if( ftruncate(fd, totalSize) == -1){
      perror("ftruncate");
      close(fd);
      return NULL;
   }
   // Use mmap -> Conect the shared Memory with a pointer from our program
   // Al hacer game_state_t *ptr, estamos tratando a la memoria como un struct (ej: ptr->a --> primeros 4 bytes, y asi)
   game_state_t * ptr = mmap(NULL, totalSize, proteccions, mapFlag, fd, 0); 
   if (ptr == MAP_FAILED) {
      perror("mmap");
      close(fd);
      return NULL;
   }
   close(fd);
   return ptr;
}

void initializeBoard(int8_t board[], uint16_t height, uint16_t width){

   srand(time(NULL));
   for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
         board[ 4*i + j ] = random_1_9();
      }
   }
   return;
}

void printBoard(int8_t board[], uint16_t height, uint16_t width){
    
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            printf("%d ",board[ 4*i + j] ); //De izq a der y de arriba a abajo (en ese orden)
        }
        printf("\n");
    }
    return;
}
   
int random_1_9() {
   return (rand() % 9) + 1;
}
