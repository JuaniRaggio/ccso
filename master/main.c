#include <game_state.h>
#include <stdint.h>
#include <stdbool.h>

#define PERMISSIONS 0666
#define MAPFLAG MAP_SHARED
#define PROTECTIONS PROT_READ | PROT_WRITE

// #define OPENFLAGS O_CREAT | O_RDWR Generates Confusission -> The master use
// both of them, the other processes only use O_RDWR (ftruncate)

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

int main(int argc, char *argv[]) {

   /*
   We need a function that Store the parameters that the user entered via STDIN.
   First we save them (localVars or struct), then  we create sharedMemory, after
   that we save them in the sharedMemory struct
   */
   int height = 3, width = 24,
       players_count = 8; // This should not be here ( We need a function to get
                          // them from stdin)

   // We create the sharedMemory for "game state" and assign a pointer to its
   // beginning

   int totalSize = (sizeof(game_state_t) + sizeof(int8_t) * height * width);
   game_state_t *sharedGameState =
       createSharedMemory("/game_state", totalSize, O_CREAT | O_RDWR, PERMISSIONS, PROTECTIONS, MAPFLAG);
   // When we asign the sharedMemory to a struct pointer, the sharedMemory
   // beahaves like a the struct (sharedGameState->...)

   // We create the sharedMemory for "game sync" and assign a pointer to its
   // beginning
   game_sync_t *sharedGameSync =
       createSharedMemory("/game_sync", sizeof(game_sync_t), O_CREAT | O_RDWR, PERMISSIONS, PROTECTIONS, MAPFLAG);

   initalizeGameState(sharedGameState, width, height, players_count);

   printGameState(sharedGameState->board, sharedGameState->height, sharedGameState->width,
                  sharedGameState->players_count, sharedGameState->state);

   // We initalize and print the sem A value, to check if the sharedMemory was
   // created propierly and the value was saved correctly
   sem_t *tmp = sem_open(NULL, 1, 231);
   sharedGameSync->A = *tmp;
   sem_post(&sharedGameSync->A); // Increase the sem (232)
   int value;
   sem_getvalue(&sharedGameSync->A, &value);
   printf("Valor del semáforo: %d\n", value);
}
