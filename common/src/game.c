#include "game_state.h"
#include "game_sync.h"
#include "shmemory_utils.h"
#include <game.h>
#include <stddef.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

typedef enum {
   game_state,
   game_sync,
   game_posible_memories,
} memory_t;

// game's users shouldn't care about this values
static const char *const default_view_path = "";
static const uint64_t default_width = 10;
static const uint64_t default_heigh = 10;
static const uint64_t default_delay = 200;
static const uint64_t default_timeout = 10;

static const uint64_t master_permissions = 0666;
static const uint64_t player_permissions = 0111;

static const shm_data_t entity_spec[total_entities][game_posible_memories] = {
    [master] =
        {
            [game_state] =
                {
                    .sharedMemoryName = game_state_memory_name,
                    .totalSize = 1, // TODO
                    .mapFlag = MAP_SHARED,
                    .openFlags = O_CREAT | O_RDWR,
                    .permissions = master_permissions,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .sharedMemoryName = game_sync_memory_name,
                    .totalSize = sizeof(game_sync_t),
                    .mapFlag = MAP_SHARED,
                    .openFlags = O_CREAT | O_RDWR,
                    .permissions = master_permissions,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
        },

    [player] =
        {
            [game_state] =
                {
                    .sharedMemoryName = game_state_memory_name,
                    .totalSize = 1, // TODO
                    .mapFlag = MAP_SHARED,
                    .openFlags = O_RDONLY,
                    .permissions = player_permissions,
                    .protections = PROT_READ,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .sharedMemoryName = game_sync_memory_name,
                    .totalSize =
                        sizeof(game_sync_t), // TODO: Podriamos usar tambien la estrategia de struct de size variable
                    .mapFlag = MAP_SHARED,
                    .openFlags = O_RDWR,
                    .permissions = player_permissions,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
        },

    // TODO: Todavia no llegamos a crear memoria para las vistas
    [view] =
        {
            [game_state] =
                {
                    .sharedMemoryName = NULL,
                    .totalSize = NULL, // TODO
                    .mapFlag = NULL,
                    .openFlags = NULL,
                    .permissions = NULL,
                    .protections = NULL,
                    .offset = NULL,

                },
            [game_sync] =
                {
                    .sharedMemoryName = NULL,
                    .totalSize = NULL, // TODO
                    .mapFlag = NULL,
                    .openFlags = NULL,
                    .permissions = NULL,
                    .protections = NULL,
                    .offset = NULL,
                },
        },
};

game_t new_game(char *argv[], int argc, entity_t who) {

   return (game_t){
       .state =
           , .sync =, .reference_count =,
   };
}

// === Inicializacion de player ===
//     manage_error, __FILE__, __func__, __LINE__);
// === Inicializacion de master ===
// game_state_t *sharedGameState = createSharedMemory(
//     manage_error, __FILE__, __func__, __LINE__);
//
// game_sync_t *sharedGameSync = createSharedMemory(
//     manage_error, __FILE__, __func__, __LINE__);

game_t game_connect(uint32_t w, uint32_t h) {}

void game_disconnect(game_t game) {
   shm_unlink(game_state_memory_name);
   shm_unlink(game_sync_memory_name);

   // Ownership of this memory depends on how many references are left
   if (--game->reference_count == 0) {
   }
   game->state = NULL;
   game->sync = NULL;
}

void delete_game(game_t game) {
   game_disconnect(game);
}

void game_end(game_t);
