/**
 * @file view.h
 * @brief ncurses-based game visualisation.
 *
 * Provides the view process's public API: initialisation, the render
 * loop that synchronises with the master via semaphores, the end-of-game
 * results screen, and cleanup.
 */
#pragma once

#include <game.h>
#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <stdint.h>

#define COLOR_BOARD 10
#define COLOR_EMPTY 11
#define COLOR_PAIR_OFFSET 1

#define CELL_WIDTH 3
#define PANEL_HEIGHT 5
#define LEADERBOARD_LABEL_Y 0
#define PLAYER_PANEL_Y_OFFSET 1
#define LABEL_BUFFER_SIZE 32
#define PLAYER_PREFIX "player-"
#define PLAYER_PREFIX_LEN 7

#define MIN_PANEL_WIDTH 12
#define STATUS_BUFFER_SIZE 16
#define LINE_BUFFER_SIZE 64
#define WIDE_BUFFER_SIZE 128

#define ENDSCREEN_WIDTH 52
#define ENDSCREEN_HEIGHT_OFFSET 8
#define ENDSCREEN_TITLE_Y_OFFSET 1
#define ENDSCREEN_WINNER_Y_OFFSET 2
#define ENDSCREEN_TABLE_Y_OFFSET 4
#define ENDSCREEN_PROMPT_Y_OFFSET 2
#define ENDSCREEN_TABLE_PADDING 12

static const int8_t NO_PLAYER = -1;

typedef struct {
    const char *head;
    const char *flag;
} player_theme_t;

static const player_theme_t PLAYER_THEMES[] = {
    {"🍚", "🇰🇷"}, // Korea
    {"🐉", "🇨🇳"}, // China
    {"🍕", "🇮🇹"}, // Italy
    {"☕", "🇬🇧"}, // UK
    {"⚽", "🇧🇷"}, // Brazil
    {"🦜", "🇨🇷"}, // Costa Rica
    {"🧉", "🇦🇷"}, // Argentina
    {"🍺", "🇩🇪"}, // Germany
    {"🐂", "🇪🇸"}, // Spain
    {"🥐", "🇫🇷"}, // France
};

typedef struct {
    WINDOW *board_win;
    WINDOW *panel_win;
    int16_t term_rows;
    int16_t term_cols;
    int16_t board_rows;
    int16_t board_x_offset;
    int16_t board_y_offset;
    uint32_t frame_count;
} view_t;

/**
 * @brief Initialise the ncurses environment and create display windows.
 *
 * Sets up locale, terminal capabilities, colour pairs, and creates the
 * board and panel windows sized to fit the given board dimensions.
 *
 * @param view         View state to initialise.
 * @param board_width  Board width in cells.
 * @param board_height Board height in cells.
 */
void view_init(view_t *view, uint16_t board_width, uint16_t board_height);

/**
 * @brief Tear down ncurses and free display windows.
 *
 * @param view View state to clean up.
 */
void view_cleanup(view_t *view);

/**
 * @brief Run the view's render loop, synchronised with the master.
 *
 * Waits for @ref game_sync_view_wait_frame, draws the board and panels,
 * then signals @ref game_sync_view_frame_done. Exits when the game
 * stops or a signal is caught.
 *
 * @param view   View state.
 * @param game   Game handle (state + sync).
 * @param width  Board width in cells.
 * @param height Board height in cells.
 */
void view_run(view_t *view, game_t *game, uint16_t width, uint16_t height);

/**
 * @brief Display the final results screen and wait for a keypress.
 *
 * Shows the leaderboard with final scores and waits for the user
 * to press any key before returning (unless interrupted).
 *
 * @param view  View state.
 * @param state Game state with final player statistics.
 */
void view_show_results(view_t *view, game_state_t *state);
