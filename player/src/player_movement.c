#include <player_movement.h>

#ifdef NAIVE

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height) {
   return rand() % 8;
}

#elif BFS

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height) {}

#elif DFS

#elif MINIMAX

#endif
