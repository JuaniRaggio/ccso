#pragma once

#include <semaphore.h>

const char *const game_sync_memory_name = "/game_sync";

typedef struct {
   sem_t pending_changes_mutex; // El máster le indica a la vista que hay cambios por imprimir
   sem_t printing_mutex;        // La vista le indica al máster que terminó de imprimir
   sem_t master_mutex;          // Mutex para evitar inanición del máster al acceder al estado
   sem_t gamestate_mutex;
   sem_t readers_count_mutex;
   unsigned int readers_count; // Cantidad de jugadores leyendo el estado
   sem_t may_send_movement[9];
} game_sync_t;
