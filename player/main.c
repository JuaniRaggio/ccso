#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>

#include <player_movement.h>
#include <game_state.h>
#include <game_sync.h>
#include <shmemory_utils.h>
#include <error_management.h>

int main(int argc, char *argv[]) {
   if (argc < 3) {
      fprintf(stderr, "Use: %s <width> <height>\n", argv[0]);
      return 1;
   }
   uint16_t width = atoi(argv[1]);
   uint16_t height = atoi(argv[2]);
   size_t totalSize = sizeof(game_state_t) + (size_t)width * height;

   game_t game = new_game(player, manage_error, __FILE__, __func__, __LINE__);

   pid_t my_pid = getpid();
   if (!is_player_ingame(&game, my_pid)) {
   }

   if (idx < 0) {
      fprintf(stderr, " PID %d not found in players' list.\n", my_pid);
      return 1;
   }

   fprintf(stderr, "I'm player %d\nInitial position: (%d,%d)\n", idx, gameState->players[idx].x,
           gameState->players[idx].y);

   // algo habría que cambiar, no conviene que se llame state.

   while (!gameState->running) { // se considera a state como juego_terminado
      // Esperar que el master me habilite para enviar un movimiento
      sem_wait(&gameSync->player_may_send_movement[idx]);

      if (gameState->running)
         break;

      sem_wait(&gameSync->master_writing);
      sem_post(&gameSync->master_writing);

      sem_wait(&gameSync->readers_count_mutex);
      if (++gameSync->readers_count == 1)
         sem_wait(&gameSync->gamestate_mutex);
      sem_post(&gameSync->readers_count_mutex);

      // Decidir movimiento mirando el tablero
      uint8_t mov =
          compute_next_move(gameState->board, width, height, gameState->players[idx].x, gameState->players[idx].y);

      // --- Liberar lectura ---
      sem_wait(&gameSync->E);
      if (--gameSync->F == 0)
         sem_post(&gameSync->D); // ultimo lector libera escritores
      sem_post(&gameSync->E);

      // Enviar movimiento al master por stdout (que es el pipe)
      write(1, &mov, 1);
   }

   munmap(gameState, totalSize);
   munmap(gameSync, sizeof(game_sync_t));
   return 0;
}
