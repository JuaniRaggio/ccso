#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

const char * const game_state_memory_name = "/game_state";
#define MAX_PLAYERS 9
#define MAX_NAME_LENGTH 16

typedef struct {
   char name[MAX_NAME_LENGTH];
   uint32_t score;
   uint32_t invalid_moves;
   uint32_t valid_moves;
   uint16_t x;      // Random
   uint16_t y;      // Random
   pid_t player_id; //> Each player will be a separate process
   bool state;
} player_t;

typedef struct {
   uint16_t width;
   uint16_t height;
   int8_t players_count;
   player_t players[MAX_PLAYERS];
   bool state;
   int8_t board[]; // tablero. fila-0, fila-1, ..., fila-n-1
} game_state_t;
