
#include <stdio.h>
#include <stdlib.h>
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

// esta en game_state.h
/*
typedef struct {
   char nombre[16];
   unsigned int puntaje;
   unsigned int mov_invalidos;
   unsigned int mov_validos;
   unsigned short x, y;
   pid_t pid;
   bool bloqueado;
} Jugador;

typedef struct {
   unsigned short ancho;
   unsigned short alto;
   unsigned char cant_jugadores;
   Jugador jugadores[9];
   bool juego_terminado;
   char tablero[];
} Estado;
*/

// está en game_sync
/*
typedef struct {
   sem_t A;
   sem_t B;
   sem_t C;
   sem_t D;
   sem_t E;
   unsigned int F;
   sem_t G[9];
} Sync;
*/

int main(int argc, char *argv[]) {
   if (argc < 3) {
      fprintf(stderr, "Use: %s <width> <height>\n", argv[0]);
      return 1;
   }
   uint16_t width = atoi(argv[1]);
   uint16_t height = atoi(argv[2]);
   size_t tam_estado = sizeof(game_state_t) + (size_t)width * height;

   // Abrir shared memory del estado (solo lectura)
   int32_t fd_state = shm_open("/game_state", O_RDONLY, 0);
   if (fd_state < 0) {
      perror("shm_open /game_state");
      return 1;
   }
   // Mapear el estado del juego en memoria, le pasamos el tamaño del estado, permisos, tipo de mapeo, fd y offset.
   game_state_t *state = mmap(NULL, tam_estado, PROT_READ, MAP_SHARED, fd_state, 0);
   if (state == MAP_FAILED) {
      perror("mmap state");
      return 1;
   }
   close(fd_state);

   // Abrir shared memory de sincronizacion (lectura/escritura para los semaforos)
   int32_t fd_sync = shm_open("/game_sync", O_RDWR, 0);
   if (fd_sync < 0) {
      perror("shm_open /game_sync");
      return 1;
   }
   game_sync_t *sync = mmap(NULL, sizeof(game_sync_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_sync, 0);
   if (sync == MAP_FAILED) {
      perror("mmap sync");
      return 1;
   }
   close(fd_sync);

   // Buscar mi indice por PID
   // El master hace fork->exec, puede que el PID todavia no este cargado
   // en la shm cuando arrancamos, por eso reintentamos un poco
   pid_t my_pid = getpid();
   int16_t idx = -1;
   for (int intento = 0; intento < 1000 && idx < 0; intento++) {
      for (int i = 0; i < state->players_count; i++) {
         if (state->players[i].player_id == my_pid) {
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

   fprintf(stderr, "I'm player %d\nInitial position: (%d,%d)\n", idx, state->players[idx].x, state->players[idx].y);

   // algo habría que cambiar, no conviene que se llame state.

   while (!state->state) { // se considera a state como juego_terminado
      // Esperar que el master me habilite para enviar un movimiento
      sem_wait(&sync->G[idx]);

      if (state->state)
         break;

      // --- Adquirir lectura (readers-writers sin inanicion del escritor) ---
      sem_wait(&sync->C); // me bloqueo si hay un escritor esperando
      sem_wait(&sync->E);
      sync->F++;
      if (sync->F == 1)
         sem_wait(&sync->D); // primer lector bloquea escritores
      sem_post(&sync->E);
      sem_post(&sync->C);

      // Decidir movimiento mirando el tablero
      uint8_t mov = compute_next_move(state, width, height, idx);

      // --- Liberar lectura ---
      sem_wait(&sync->E);
      sync->F--;
      if (sync->F == 0)
         sem_post(&sync->D); // ultimo lector libera escritores
      sem_post(&sync->E);

      // Enviar movimiento al master por stdout (que es el pipe)
      write(1, &mov, 1);
   }

   munmap(state, tam_estado);
   munmap(sync, sizeof(game_sync_t));
   return 0;
}

// viejo main.c
/*
#include <stdio.h>

#include <player_movement.h>

int main(int argc, char *argv[]) {
   printf("Player process started\n");
   return 0;
}
*/