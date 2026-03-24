#include <player_movement.h>

// #ifdef NAIVE

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
   return rand() % 8;
}

/*
#elif BFS

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height,  uint16_t x, uint16_t y) {}

#elif DFS

#elif MINIMAX

#elif A_STAR

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height,  uint16_t x, unit16_t y) {}

*/
// #endif

// Deltas para las 8 direcciones: 0=arriba, 1=arriba-der, 2=der, 3=abajo-der,
//                                 4=abajo,  5=abajo-izq,  6=izq, 7=arriba-izq
static const int DX[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

// del mockup
int8_t decidir_movimiento(game_state_t *state, uint16_t width, uint16_t height, uint16_t idx) {
   int x = state->players[idx].x;
   int y = state->players[idx].y;

   for (int dir = 0; dir < 8; dir++) {
      int nx = x + DX[dir];
      int ny = y + DY[dir];

      // Fuera del tablero
      if (nx < 0 || nx >= width || ny < 0 || ny >= height)
         continue;

      // Celda libre: valor entre 1 y 9
      uint8_t cell = state->board[ny * width + nx];
      if (cell >= 1 && cell <= 9)
         return (uint8_t)dir;
   }

   // Sin movimiento válido — mandamos algo inválido, el master lo ignora
   // y eventualmente cierra el juego por timeout o bloqueado
   return 0;
}