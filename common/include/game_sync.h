#pragma once

#include "game_state.h"
#include <semaphore.h>
#include <stdint.h>

const char *const game_sync_memory_name = "/game_sync";

typedef struct {
   sem_t pending_changes_to_show; // El máster le indica a la vista que hay cambios por imprimir
   sem_t printing;        // La vista le indica al máster que terminó de imprimir
   sem_t master_writing;          // Mutex para evitar inanición del máster al acceder al estado
   sem_t gamestate_mutex;
   sem_t readers_count_mutex;
   uint32_t readers_count; // Cantidad de jugadores leyendo el estado
   sem_t player_may_send_movement[MAX_PLAYERS];
} game_sync_t;
