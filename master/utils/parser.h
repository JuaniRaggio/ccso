#pragma once

// === Parametros a soportar ===
// [-w width]: Ancho del tablero. Default y mínimo: 10
// [-h height]: Alto del tablero. Default y mínimo: 10
// [-d delay]: milisegundos que espera el máster cada vez que se imprime el estado. Default: 200
// [-t timeout]: Timeout en segundos para recibir solicitudes de movimientos válidos. Default: 10
// [-s seed]: Semilla utilizada para la generación del tablero. Default: time(NULL)
// [-v view]: Ruta del binario de la vista. Default: Sin vista.
// -p player1 player2: Ruta/s de los binarios de los jugadores. Mínimo: 1, Máximo: 9.
//
// ./binario -w 10 -h 1000 -d 400

#include "game_state.h"
#include <stdint.h>

typedef struct {
   uint16_t players_count;
   uint16_t seed;
   uint16_t width;
   uint16_t height;
   uint32_t delay;
   uint32_t timeout;
   const char *view_path;
   const char *players_paths[MAX_PLAYERS];
} parameters_t;

typedef enum {
   success = 0x0,
   invalid_width_type = success << 1,
   invalid_height_type = invalid_width_type << 1,
   invalid_delay_type =  invalid_height_type << 1,
   invalid_timeout_type = invalid_delay_type << 1,
   invalid_seed_type = invalid_timeout_type << 1,
   invalid_view_path = invalid_seed_type << 1,
   invalid_player_count = invalid_view_path << 1,
   invalid_width_limit = invalid_player_count << 1,
   invalid_height_limit = invalid_width_limit << 1,
} parameter_status_t;

parameter_status_t parse(int argc, char *argv[], parameters_t *parameters);
