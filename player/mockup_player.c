// este es un player que funcionó con el ChompChamps de la cátedra sin tirar movimientos inálidos.

/*
- para compilarlo: gcc -Wall -o mockup_player mockup_player.c -lrt -lpthread
- para correrlo con ChompChamps: ./ChompChamps -w 11 -h 11 -p ./mockup_player -i -s 100
- resultado:  Soy jugador 0, posicion inicial (5,5)
Player mockup_player (0) exited (0) with a score of 176 / 36 / 0
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <player_movement.h>

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

typedef struct {
    sem_t A;
    sem_t B;
    sem_t C;
    sem_t D;
    sem_t E;
    unsigned int F;
    sem_t G[9];
} Sync;


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Use: %s <width> <height>\n", argv[0]);
        return 1;
    }
    uint16_t width = atoi(argv[1]);
    uint16_t height  = atoi(argv[2]);
    size_t tam_estado = sizeof(Estado) + (size_t)width * height;

    // Abrir shared memory del estado (solo lectura)
    int fd_e = shm_open("/game_state", O_RDONLY, 0);
    if (fd_e < 0) { perror("shm_open /game_state"); return 1; }
    Estado *estado = mmap(NULL, tam_estado, PROT_READ, MAP_SHARED, fd_e, 0);
    if (estado == MAP_FAILED) { perror("mmap estado"); return 1; }
    close(fd_e);

    // Abrir shared memory de sincronizacion (lectura/escritura para los semaforos)
    int fd_s = shm_open("/game_sync", O_RDWR, 0);
    if (fd_s < 0) { perror("shm_open /game_sync"); return 1; }
    Sync *sync = mmap(NULL, sizeof(Sync), PROT_READ|PROT_WRITE, MAP_SHARED, fd_s, 0);
    if (sync == MAP_FAILED) { perror("mmap sync"); return 1; }
    close(fd_s);

    // Buscar mi indice por PID
    // El master hace fork->exec, puede que el PID todavia no este cargado
    // en la shm cuando arrancamos, por eso reintentamos un poco
    pid_t mi_pid = getpid();
    int idx = -1;
    for (int intento = 0; intento < 1000 && idx < 0; intento++) {
        for (int i = 0; i < estado->cant_jugadores; i++) {
            if (estado->jugadores[i].pid == mi_pid) {
                idx = i;
                break;
            }
        }
        if (idx < 0) usleep(1000); // esperar 1ms y reintentar
    }
    if (idx < 0) {
        fprintf(stderr, "No encontre mi PID %d en la lista de jugadores\n", mi_pid);
        return 1;
    }

    fprintf(stderr, "Soy jugador %d, posicion inicial (%d,%d)\n",
            idx, estado->jugadores[idx].x, estado->jugadores[idx].y);

    while (!estado->juego_terminado) {
        // Esperar que el master me habilite para enviar un movimiento
        sem_wait(&sync->G[idx]);

        if (estado->juego_terminado) break;

        // --- Adquirir lectura (readers-writers sin inanicion del escritor) ---
        sem_wait(&sync->C);       // me bloqueo si hay un escritor esperando
        sem_wait(&sync->E);
        sync->F++;
        if (sync->F == 1) sem_wait(&sync->D); // primer lector bloquea escritores
        sem_post(&sync->E);
        sem_post(&sync->C);

        // Decidir movimiento mirando el tablero
        uint8_t mov = decidir_movimiento(estado, ancho, alto, idx);

        // --- Liberar lectura ---
        sem_wait(&sync->E);
        sync->F--;
        if (sync->F == 0) sem_post(&sync->D); // ultimo lector libera escritores
        sem_post(&sync->E);

        // Enviar movimiento al master por stdout (que es el pipe)
        write(1, &mov, 1);
    }

    munmap(estado, tam_estado);
    munmap(sync, sizeof(Sync));
    return 0;
}