#include <stdint.h>
#include <stdbool.h>
#include <parser.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "game_state.h"
#include "game_sync.h"
#include "master.h"

#define PERMISSIONS 0666
#define MAPFLAG MAP_SHARED
#define PROTECTIONS PROT_READ | PROT_WRITE

// #define OPENFLAGS O_CREAT | O_RDWR Generates Confusission -> The master use
// both of them, the other processes only use O_RDWR (ftruncate)

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

// === Parametros a soportar ===
// [-w parameters.width]: Ancho del tablero. Default y mínimo: 10
// [-h parameters.height]: Alto del tablero. Default y mínimo: 10
// [-d delay]: milisegundos que espera el máster cada vez que se imprime el estado. Default: 200
// [-t timeout]: Timeout en segundos para recibir solicitudes de movimientos válidos. Default: 10
// [-s seed]: Semilla utilizada para la generación del tablero. Default: time(NULL)
// [-v view]: Ruta del binario de la vista. Default: Sin vista.
// -p player1 player2: Ruta/s de los binarios de los jugadores. Mínimo: 1, Máximo: 9.

int main(int argc, char *argv[]) {

   parameters_t parameters = (parameters_t){
       .width = 10,
       .height = 10,
       .delay = 200,
       .timeout = 10,
       .seed = time(NULL),
       .view_path = "",
   };

   // We create the sharedMemory for "game state" and assign a pointer to its
   // beginning

   int totalSize = (sizeof(game_state_t) + sizeof(int8_t) * parameters.height * parameters.width);
   game_state_t *sharedGameState =
       createSharedMemory("/game_state", totalSize, O_CREAT | O_RDWR, PERMISSIONS, PROTECTIONS, MAPFLAG);
   // When we asign the sharedMemory to a struct pointer, the sharedMemory
   // beahaves like a the struct (sharedGameState->...)

   // We create the sharedMemory for "game sync" and assign a pointer to its
   // beginning
   game_sync_t *sharedGameSync =
       createSharedMemory("/game_sync", sizeof(game_sync_t), O_CREAT | O_RDWR, PERMISSIONS, PROTECTIONS, MAPFLAG);

   initalizeGameState(sharedGameState, parameters.width, parameters.height, players_count);

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
