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

static const size_t uninitialized_size = 0;

//> IMPORTANT: totalSize is not compile-time computable for game_state
//> since it's value depends on width & height => When entity_spec is
//> used, you need to override this value on the copy
static const shm_data_t entity_spec[total_entities][game_posible_memories] = {
    [master] =
        {
            [game_state] =
                {
                    .sharedMemoryName = game_state_memory_name,
                    .mapFlag = MAP_SHARED,
                    .openFlags = O_CREAT | O_RDWR,
                    .permissions = master_permissions,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .sharedMemoryName = game_sync_memory_name,
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
                    .mapFlag = MAP_SHARED,
                    .openFlags = O_RDONLY,
                    .permissions = player_permissions,
                    .protections = PROT_READ,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .sharedMemoryName = game_sync_memory_name,
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
                    .mapFlag = NULL,
                    .openFlags = NULL,
                    .permissions = NULL,
                    .protections = NULL,
                    .offset = NULL,
                },
            [game_sync] =
                {
                    .sharedMemoryName = NULL,
                    .mapFlag = NULL,
                    .openFlags = NULL,
                    .permissions = NULL,
                    .protections = NULL,
                    .offset = NULL,
                },
        },
};

static void init_game_state(game_t *game) {}

static void init_game_sync(game_t *game) {}

static void init_game(game_t *game) {
    init_game_state(game);
    init_game_sync(game);
}

game_t _new_game(entity_t who, game_params_t game_parameters) {
    if (who >= total_entities) {
        game_parameters.manage_error(game_parameters.file, game_parameters.func, game_parameters.line, entity_error);
    }
    size_t state_size = sizeof(game_state_t) + game_parameters.width * game_parameters.height;
    size_t sync_size = sizeof(game_sync_t);
    game_t game = {
        .state = createSharedMemory(&entity_spec[who][game_state], state_size, game_parameters.manage_error,
                                    game_parameters.file, game_parameters.func, game_parameters.line),
        .sync = createSharedMemory(&entity_spec[who][game_sync], sync_size, game_parameters.manage_error,
                                   game_parameters.file, game_parameters.func, game_parameters.line),
        .shm_total_size = state_size + sync_size,
    };

    if (who == master) {
        init_game(&game);
    }

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
