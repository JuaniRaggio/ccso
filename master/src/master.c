#include <stdint.h>
#include <stdio.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <game_state.h>
#include <game_sync.h>
#include "master.h"

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width);

// TODO init players
void initalizeGameState(game_state_t *sharedGameState, uint16_t width, uint16_t height, int8_t players_count,
                        player_t players[MAX_PLAYERS]) {
    sharedGameState->width = width;
    sharedGameState->height = height;
    sharedGameState->players_count = players_count;
    sharedGameState->state = 0;
    initializeBoard(sharedGameState->board, height, width);
    memcpy(sharedGameState->players, players, sizeof(player_t) * players_count);
}

// We initialize the board with random numbers between 1 and 9
void initializeBoard(int8_t board[], uint16_t height, uint16_t width) {
    srand(time(NULL));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[4 * i + j] = random_1_9();
        }
    }
    return;
}

int random_1_9() {
    return (rand() % 9) + 1;
}

// -----------------------------------------------------------------------------------------------------------------

void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state) {

    printf("WIDTH: %d\nHEIGHT: %d\nPLAYER COUNT: %d\nSTATE: %d\n\nBOARD:\n", width, height, players_count, state);
    printBoard(board, height, width);
}

void printBoard(int8_t board[], uint16_t height, uint16_t width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            printf("%d ",
                   board[4 * i + j]); // De izq a der y de arriba a abajo (en ese orden)
        }
        printf("\n");
    }
    printf("\n");
    return;
}

void printGameSync(game_sync_t *sync) {

    int val;

    fprintf(stderr, "\n=== GAME SYNC ===\n");

    sem_getvalue(&sync->A, &val);
    fprintf(stderr, "A: %d\n", val);

    sem_getvalue(&sync->B, &val);
    fprintf(stderr, "B: %d\n", val);

    sem_getvalue(&sync->C, &val);
    fprintf(stderr, "C: %d\n", val);

    sem_getvalue(&sync->D, &val);
    fprintf(stderr, "D: %d\n", val);

    sem_getvalue(&sync->E, &val);
    fprintf(stderr, "E: %d\n", val);

    fprintf(stderr, "F (lectores): %u\n", sync->F);

    for (int i = 0; i < 9; i++) {
        sem_getvalue(&sync->G[i], &val);
        fprintf(stderr, "G[%d]: %d\n", i, val);
    }

    fprintf(stderr, "=================\n\n");
}

// ------------------------------------------------------------------------------------------------------------------
