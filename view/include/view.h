#pragma once

#define COLOR_BOARD   10
#define COLOR_EMPTY   11

#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <stdint.h>

static const int16_t HEX_CELL_WIDTH = 5;
static const int16_t HEX_CELL_BASE_HEIGHT = 3;
static const int16_t HEX_MAX_ELEVATION = 4;

static const int16_t PANEL_WIDTH = 28;
static const int16_t PANEL_HEIGHT = 5; // border + face/score + last move + delay + border

static const char *const PLAYER_FACES[] = {
    "(:)",
    "(;)",
    "(:P",
    "(>)",
    "(<)",
    "(:O",
    "(:|",
    "(:D",
    "(:/",
};

static const char *const DIR_NAMES[] = {
    "UP", "UP-RIGHT", "RIGHT", "DOWN-RIGHT",
    "DOWN", "DOWN-LEFT", "LEFT", "UP-LEFT",
};

typedef struct {
    WINDOW *board_win;   // window for the hex grid
    WINDOW *panel_win;   // window for player info panels
    int16_t term_rows;
    int16_t term_cols;
} view_t;

void view_init(view_t *view);
void view_cleanup(view_t *view);
void view_draw_board(view_t *view, game_state_t *state);
void view_draw_panels(view_t *view, game_state_t *state);
void view_draw_all(view_t *view, game_state_t *state);
