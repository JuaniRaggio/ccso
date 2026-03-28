#include <stdint.h>
#include <stdbool.h>
#include <parser.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "game_state.h"
#include "game_sync.h"
#include "master.h"
#include "shmemory_utils.h"
#include <error_management.h>

static const uint64_t master_permissions = 0666;

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

int main(int argc, char *argv[]) {
   errno = 0;
   parameters_t parameters = (parameters_t){
       .width = 10,
       .height = 10,
       .delay = 200,
       .timeout = 10,
       .seed = time(NULL),
       .view_path = "",
   };

   parameter_status_t status = parse(argc, argv, &parameters);

   if (status != success) {
      // Manage errors using status
   }

   errno = 0;
   size_t totalSize = (sizeof(game_state_t) + sizeof(int8_t) * parameters.height * parameters.width);
   game_state_t *sharedGameState = createSharedMemory(
       &(shm_data_t){
           .sharedMemoryName = "/game_state",
           .offset = 0,
           .totalSize = totalSize,
           .protections = PROT_READ | PROT_WRITE,
           .mapFlag = MAP_SHARED,
           .permissions = master_permissions,
           .openFlags = O_CREAT | O_RDWR,
       },
       manage_error, __FILE__, __func__, __LINE__);

   game_sync_t *sharedGameSync = createSharedMemory(
       &(shm_data_t){
           .sharedMemoryName = "/game_sync",
           .totalSize = sizeof(game_sync_t),
           .permissions = master_permissions,
           .protections = PROT_READ | PROT_WRITE,
           .mapFlag = MAP_SHARED,
           .offset = 0,
           .openFlags = O_CREAT | O_RDWR,
       },
       manage_error, __FILE__, __func__, __LINE__);

   player_t players[MAX_PLAYERS] = {};
   initalizeGameState(sharedGameState, parameters.width, parameters.height, parameters.players_count, players);

   printGameState(sharedGameState->board, sharedGameState->height, sharedGameState->width,
                  sharedGameState->players_count, sharedGameState->state);
}
