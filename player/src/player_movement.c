#include <player_movement.h>
#include <string.h>

static const int DX[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

// Que tanto suma la calidad de los jugadores?
// No suma absolutamente nada, importante calidad de codigo y que
// funcione
//
// Para la vista lo mismo
//
// Si suma como beneficio si gana el torneo
//
//
// Podemos hacer un if que dependiendo de la cantidad de jugadores
// se tome una strat u otra
#define NO_VALID_MOVE 0

static inline int in_bounds(int x, int y, uint16_t width, uint16_t height) {
   return x >= 0 && x < width && y >= 0 && y < height;
}

static inline int8_t board_at(int8_t board[], uint16_t width, int x, int y) {
   return board[y * width + x];
}

static inline int is_free(int8_t cell) {
   return cell >= 1 && cell <= 9;
}

static inline void neighbor(int x, int y, int dir, int *nx, int *ny) {
   *nx = x + DX[dir];
   *ny = y + DY[dir];
}

static inline int is_free_neighbor(int8_t board[], uint16_t width, uint16_t height, int x, int y, int dir,
                                   int *out_nx, int *out_ny) {
   neighbor(x, y, dir, out_nx, out_ny);
   return in_bounds(*out_nx, *out_ny, width, height) && is_free(board_at(board, width, *out_nx, *out_ny));
}

#if defined(FLOOD) || defined(GREEDY_FLOOD)

static int flood_count(int8_t board[], uint16_t width, uint16_t height, int sx, int sy) {
   int total = width * height;
   int count = 0;
   return count;
}

#endif

#ifdef NAIVE

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
   return rand() % 8;
}

#elif defined(GREEDY)

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
   int best_dir = -1;
   int8_t best_val = 0;
   for (int dir = 0; dir < 8; dir++) {
      int nx, ny;
      if (!is_free_neighbor(board, width, height, x, y, dir, &nx, &ny))
         continue;
      int8_t cell = board_at(board, width, nx, ny);
      if (cell > best_val) {
         best_val = cell;
         best_dir = dir;
      }
   }
   return best_dir >= 0 ? best_dir : NO_VALID_MOVE;
}

#elif defined(GREEDY_LOOKAHEAD)

#define LOOKAHEAD_DEPTH 3

static int path_contains(int path_x[], int path_y[], int path_len, int x, int y) {
}

static int lookahead(int8_t board[], uint16_t width, uint16_t height, int cx, int cy, int depth,
                     int path_x[], int path_y[], int path_len) {
}

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
}

#elif defined(FLOOD)

#elif defined(GREEDY_FLOOD)

// del mockup
#endif

int8_t decidir_movimiento(game_state_t *state, uint16_t width, uint16_t height, uint16_t idx) {
   int x = state->players[idx].x;
   int y = state->players[idx].y;

   for (int dir = 0; dir < 8; dir++) {
      int nx, ny;
      if (!is_free_neighbor(state->board, width, height, x, y, dir, &nx, &ny))
         continue;
      return (uint8_t)dir;
   }

   return NO_VALID_MOVE;
}
