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
#include <time.h>

typedef enum {
    game_state,
    game_sync,
    game_posible_memories,
} memory_t;

// game's users shouldn't care about this values
static const uint64_t shm_create_mode = 0666;
static const uint64_t shm_unused_mode = 0;

//> IMPORTANT: totalSize is not compile-time computable for game_state
//> since it's value depends on width & height => When entity_spec is
//> used, you need to override this value on the copy
static const shm_data_t entity_spec[total_entities][game_posible_memories] = {
    [master] =
        {
            [game_state] =
                {
                    .shared_memory_name = game_state_memory_name,
                    .map_flag = MAP_SHARED,
                    .open_flags = O_CREAT | O_RDWR,
                    .permissions = shm_create_mode,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .shared_memory_name = game_sync_memory_name,
                    .map_flag = MAP_SHARED,
                    .open_flags = O_CREAT | O_RDWR,
                    .permissions = shm_create_mode,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
        },

    [player] =
        {
            [game_state] =
                {
                    .shared_memory_name = game_state_memory_name,
                    .map_flag = MAP_SHARED,
                    .open_flags = O_RDONLY,
                    .permissions = shm_unused_mode,
                    .protections = PROT_READ,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .shared_memory_name = game_sync_memory_name,
                    .map_flag = MAP_SHARED,
                    .open_flags = O_RDWR,
                    .permissions = shm_unused_mode,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
        },

    [view] =
        {
            [game_state] =
                {
                    .shared_memory_name = game_state_memory_name,
                    .map_flag = MAP_SHARED,
                    .open_flags = O_RDONLY,
                    .permissions = shm_unused_mode,
                    .protections = PROT_READ,
                    .offset = 0,
                },
            [game_sync] =
                {
                    .shared_memory_name = game_sync_memory_name,
                    .map_flag = MAP_SHARED,
                    .open_flags = O_RDWR,
                    .permissions = shm_unused_mode,
                    .protections = PROT_READ | PROT_WRITE,
                    .offset = 0,
                },
        },
};

static game_t game_create_shared_memory(entity_t who, game_params_t *game_parameters) {
    static const size_t sync_size = sizeof(game_sync_t);
    size_t state_size = sizeof(game_state_t) + game_parameters->width * game_parameters->height;
    return (game_t){
        .state = _create_shm(&entity_spec[who][game_state], state_size, game_parameters->manage_error,
                             game_parameters->file, game_parameters->func, game_parameters->line),
        .sync = _create_shm(&entity_spec[who][game_sync], sync_size, game_parameters->manage_error,
                            game_parameters->file, game_parameters->func, game_parameters->line),
        .shm_total_size = state_size + sync_size,
    };
}

game_t _new_game(entity_t who, game_params_t game_parameters) {
    if (who >= total_entities) {
        game_parameters.manage_error(game_parameters.file, game_parameters.func, game_parameters.line,
                                     invalid_argument_error);
    }
    game_t game = game_create_shared_memory(who, &game_parameters);
    if (who == master) {
        game_sync_init(game.sync);
    }
    return game;
}

void game_disconnect(game_t *game) {
    if (game == NULL || game->state == NULL) {
        manage_error(__FILE__, __func__, __LINE__, invalid_argument_error);
        return;
    }
    destroy_shm(game->state, sizeof(game_state_t) + game->state->width * game->state->height);
    destroy_shm(game->sync, sizeof(game_sync_t));
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
