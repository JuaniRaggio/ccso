#pragma once

#define COLOR_BOARD 10
#define COLOR_EMPTY 11

#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <stdint.h>

static const int16_t HEX_CELL_WIDTH = 5;
static const int16_t HEX_CELL_BASE_HEIGHT = 3;
static const int16_t HEX_MAX_ELEVATION = 4;
static const int16_t HEX_CELL_OVERLAP = 1;     // adjacent cells share 1 column
static const int16_t HEX_STAGGER_DIVISOR = 2;  // odd columns shift down by half a slot

static const int16_t HEX_TOP_LINE = 0;
static const int16_t HEX_BODY_LINE = 1;
static const int16_t HEX_BOTTOM_LINE = 2;
static const int16_t HEX_BODY_TEXT_OFFSET = 1; // text starts 1 char into the body

static const int8_t MIN_CELL_VALUE = 1;
static const int8_t MAX_CELL_VALUE = 9;
static const int8_t CELL_VALUE_RANGE = 8; // MAX - MIN
static const int16_t PANEL_WIDTH = 28;
static const int16_t PANEL_HEIGHT = 5; // border + face/score + last move + delay + border

static const int16_t MIN_BOARD_WIDTH = 20; // minimum cols to show board + panels

static const int16_t COLOR_PAIR_OFFSET = 1;

static const int8_t NO_PLAYER = -1;

#define LABEL_BUFFER_SIZE 32

static const char *const PLAYER_FACES[] = {
    "[^_^]", "[o_O]", "[>_<]", "[-_-]", "[T_T]", "[=_=]", "[*_*]", "[@_@]", "[!_!]",
};

static const char *const DIR_NAMES[] = {
    "UP", "UP-RIGHT", "RIGHT", "DOWN-RIGHT", "DOWN", "DOWN-LEFT", "LEFT", "UP-LEFT",
};

typedef struct {
    WINDOW *board_win; // window for the hex grid
    WINDOW *panel_win; // window for player info panels
    int16_t term_rows;
    int16_t term_cols;
} view_t;

void view_init(view_t *view);
void view_cleanup(view_t *view);
void view_draw_board(view_t *view, game_state_t *state);
void view_draw_panels(view_t *view, game_state_t *state);
void view_draw_all(view_t *view, game_state_t *state);
