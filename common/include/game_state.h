/**
 * @file game_state.h
 * @brief Shared game state structures stored in POSIX shared memory.
 *
 * Defines the player descriptor and the game board layout that are shared
 * between the master, player, and view processes via mmap. The board is
 * stored as a flexible array member at the end of @ref game_state_t.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/** @brief Maximum number of concurrent players in a game. */
#define MAX_PLAYERS 9

/** @brief Maximum length (including NUL) for a player name. */
#define MAX_NAME_LENGTH 16

/** @brief POSIX shared memory object name for the game state segment. */
extern const char game_state_memory_name[];

/**
 * @brief Per-player runtime information.
 *
 * Each player has a position, score counters, and an alive/dead flag.
 * Negative board cell values encode the player index that occupies the cell.
 */
typedef struct {
    char name[MAX_NAME_LENGTH]; /**< Display name (basename of the binary). */
    uint32_t score;             /**< Accumulated score from collected cells. */
    uint32_t invalid_moves;     /**< Number of invalid moves attempted. */
    uint32_t valid_moves;       /**< Number of valid moves executed. */
    uint16_t x;                 /**< Current column on the board. */
    uint16_t y;                 /**< Current row on the board. */
    pid_t player_id;            /**< PID of the player child process. */
    bool state;                 /**< true if alive, false if disconnected. */
} player_t;

/**
 * @brief Complete game state residing in shared memory.
 *
 * The @c board flexible array stores @c width * @c height cells laid out
 * row by row. Positive values [1..9] are collectible rewards; zero means
 * empty; negative values encode the player index that occupies the cell.
 */
typedef struct {
    uint16_t width;                /**< Board column count. */
    uint16_t height;               /**< Board row count. */
    int8_t players_count;          /**< Number of registered players. */
    player_t players[MAX_PLAYERS]; /**< Player descriptors. */
    bool running;                  /**< false once the game has ended. */
    int8_t board[];                /**< Row-major board: board[y * width + x]. */
} game_state_t;
