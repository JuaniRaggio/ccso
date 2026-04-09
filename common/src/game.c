#include "error_management.h"
#include "game_state.h"
#include "game_sync.h"
#include "shmemory_utils.h"
#include <errno.h>
#include <game.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

typedef enum {
    game_state,
    game_sync,
    game_posible_memories,
} memory_t;

// game's users shouldn't care about this values

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

game_t _new_game(entity_t who, error_manager_t manage_error, const char *file, const char *func, uint64_t line) {
    if (who >= total_entities) {
        manage_error(file, func, line, entity_error);
    }

    game_t game = {
        .state = createSharedMemory(&entity_spec[who][game_state], manage_error, file, func, line),
        .sync = createSharedMemory(&entity_spec[who][game_sync], manage_error, file, func, line),
    };

    return game;
}

game_t game_connect(uint32_t w, uint32_t h) {}

void game_disconnect(game_t *game) {
    if (game == NULL) {
        // TODO manage error
        return;
    }
    shm_unlink(game_state_memory_name);
    shm_unlink(game_sync_memory_name);

    // Ownership of this memory depends on how many references are left
    if (--game->reference_count == 0) {
    }
    game->state = NULL;
    game->sync = NULL;
}

void delete_game(game_t *game) {
    game_disconnect(game);
}

void game_end(game_t *) {}

uint_fast8_t players_ingame(game_t *game) {
    return game->state->players_count;
}

bool is_player_ingame(game_t *game, pid_t player_id) {
    for (uint_fast8_t idx = 0; idx < players_ingame(game); idx++) {
        if (game->state->players[idx].player_id == player_id) {
            return true;
        }
    }
    return false;
}
