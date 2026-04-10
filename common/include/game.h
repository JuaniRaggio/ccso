#pragma once

#include "error_management.h"
#include "game_state.h"
#include "game_sync.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    game_state_t *state;
    game_sync_t *sync;

    size_t shm_total_size;
} game_t;

typedef enum {
    master,
    player,
    view,
    total_entities,
} entity_t;

typedef struct {
    error_manager_t manage_error;
    const char *file;
    const char *func;
    uint64_t line;
    uint64_t seed;
    uint16_t width;
    uint16_t height;
} game_params_t;

static const char *const default_view_path = "";
static const uint64_t default_width = 10;
static const uint64_t default_heigh = 10;
static const uint64_t default_delay = 200;
static const uint64_t default_timeout = 10;

#define new_game(who, ...)                                                                                             \
    _new_game((who), (game_params_t){                                                                                  \
                         .manage_error = manage_error,                                                                 \
                         .file = __FILE__,                                                                             \
                         .func = __func__,                                                                             \
                         .line = __LINE__,                                                                             \
                         .seed = time(NULL),                                                                           \
                         .width = default_width,                                                                       \
                         .height = default_heigh,                                                                      \
                         __VA_ARGS__,                                                                                  \
                     })

game_t _new_game(entity_t who, game_params_t game_parameters);

game_t game_connect(uint32_t w, uint32_t h);
void game_disconnect(game_t *);
void game_end(game_t *);

uint_fast8_t players_ingame(game_t *game);
bool is_player_ingame(game_t *game, pid_t player_id);
