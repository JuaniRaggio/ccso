#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  char name[16];
  unsigned int score;
  unsigned int invalid_moves;
  unsigned int valid_moves;
  unsigned short x;
  unsigned short y;
  pid_t player_id; //> Each player will be a separate process
  bool state;
} player_t;

typedef struct {
  unsigned short width;
  unsigned short height;
  unsigned char players_count;
  player_t players[9];
  bool state;
  char board[]; // tablero. fila-0, fila-1, ..., fila-n-1
} game_state_t;
