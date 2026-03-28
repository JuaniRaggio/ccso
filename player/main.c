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

static const uint64_t player_permissions = 0111;

int main(int argc, char *argv[]) {
   if (argc < 3) {
      fprintf(stderr, "Use: %s <width> <height>\n", argv[0]);
      return 1;
   }
   uint16_t width = atoi(argv[1]);
   uint16_t height = atoi(argv[2]);
   size_t totalSize = sizeof(game_state_t) + (size_t)width * height;

   game_state_t *gameState = createSharedMemory(
       &(shm_data_t){
           .sharedMemoryName = game_state_memory_name,
           .totalSize = totalSize,
           .mapFlag = MAP_SHARED,
           .openFlags = O_RDONLY,
           .permissions = player_permissions,
           .protections = PROT_READ,
           .offset = 0,
       },
       manage_errno, __FILE__, __func__, __LINE__);

   game_sync_t *gameSync = createSharedMemory(
       &(shm_data_t){
           .sharedMemoryName = game_sync_memory_name,
           .totalSize = sizeof(game_sync_t),
           .mapFlag = MAP_SHARED,
           .openFlags = O_RDWR,
           .permissions = player_permissions,
           .protections = PROT_READ | PROT_WRITE,
           .offset = 0,
       },
       manage_errno, __FILE__, __func__, __LINE__);

   // Buscar mi indice por PID
   // El master hace fork->exec, puede que el PID todavia no este cargado
   // en la shm cuando arrancamos, por eso reintentamos un poco
   pid_t my_pid = getpid();
   int16_t idx = -1;
   for (int intento = 0; intento < 1000 && idx < 0; intento++) {
      for (int i = 0; i < gameState->players_count; i++) {
         if (gameState->players[i].player_id == my_pid) {
            idx = i;
            break;
         }
      }
      if (idx < 0)
         usleep(1000); // esperar 1ms y reintentar
   }
   if (idx < 0) {
      fprintf(stderr, " PID %d not found in players' list.\n", my_pid);
      return 1;
   }

   fprintf(stderr, "I'm player %d\nInitial position: (%d,%d)\n", idx, gameState->players[idx].x,
           gameState->players[idx].y);

   // algo habría que cambiar, no conviene que se llame state.

   while (!gameState->state) { // se considera a state como juego_terminado
      // Esperar que el master me habilite para enviar un movimiento
      sem_wait(&gameSync->G[idx]);

      if (gameState->state)
         break;

      // --- Adquirir lectura (readers-writers sin inanicion del escritor) ---
      sem_wait(&gameSync->C); // me bloqueo si hay un escritor esperando
      sem_wait(&gameSync->E);
      gameSync->F++;
      if (gameSync->F == 1)
         sem_wait(&gameSync->D); // primer lector bloquea escritores
      sem_post(&gameSync->E);
      sem_post(&gameSync->C);

      // Decidir movimiento mirando el tablero
      uint8_t mov =
          compute_next_move(gameState->board, width, height, gameState->players[idx].x, gameState->players[idx].y);

      // --- Liberar lectura ---
      sem_wait(&gameSync->E);
      gameSync->F--;
      if (gameSync->F == 0)
         sem_post(&gameSync->D); // ultimo lector libera escritores
      sem_post(&gameSync->E);

      // Enviar movimiento al master por stdout (que es el pipe)
      write(1, &mov, 1);
   }

   munmap(gameState, totalSize);
   munmap(gameSync, sizeof(game_sync_t));
   return 0;
}
