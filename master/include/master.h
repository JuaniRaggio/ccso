#pragma once

#include <game_state.h>
#include <shmemory_utils.h>
#include <stdio.h>

// Returns a random number between 1 and 9
int random_1_9();

// Initialize the board (height X width) with random numbers from 1 to 9
void initializeBoard(int8_t board[], uint16_t height, uint16_t width);

void initalizeGameState(game_state_t *sharedGameState, uint16_t width, uint16_t height, int8_t players_count,
                        player_t players[MAX_PLAYERS]);
