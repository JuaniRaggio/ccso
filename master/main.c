#include <stdint.h>
#include <stdbool.h>
#include <parser.h>
#include <stdio.h>
#include <sys/errno.h>
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

void manage_errno(const char *file, const char *func, uint64_t line) {
   switch (errno) {
   case EACCES:
      fprintf(stderr, "Access Error... File: %s\n Function: %s Line: %lu\n", file, func, line);
      break;
   case EEXIST:
      fprintf(stderr, "Exist Error... File: %s\n Function: %s Line: %lu\n", file, func, line);
      break;
   }
}

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
   game_state_t *sharedGameState = createSharedMemory("/game_state", totalSize, O_CREAT | O_RDWR, PERMISSIONS,
                                                      PROTECTIONS, MAPFLAG, 0 /*, manage_errno*/);

   game_sync_t *sharedGameSync = createSharedMemory("/game_sync", sizeof(game_sync_t), O_CREAT | O_RDWR, PERMISSIONS,
                                                    PROTECTIONS, MAPFLAG, 0 /*, manage_errno*/);

   player_t players[9] = {};                                          
   initalizeGameState(sharedGameState, parameters.width, parameters.height, parameters.players_count, players);

   printGameState(sharedGameState->board, sharedGameState->height, sharedGameState->width,
                  sharedGameState->players_count, sharedGameState->state);

}
