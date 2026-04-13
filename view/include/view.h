#pragma once

#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <stdint.h>

#define COLOR_BOARD 10
#define COLOR_EMPTY 11
#define COLOR_PAIR_OFFSET 1

#define CELL_WIDTH 3   // "XX " per cell
#define PANEL_HEIGHT 4 // top border + face/score + stats + bottom border
#define LABEL_BUFFER_SIZE 32
#define PLAYER_PREFIX "player-"
#define PLAYER_PREFIX_LEN 7

#define MIN_PANEL_WIDTH 12
#define STATUS_BUFFER_SIZE 16
#define LINE_BUFFER_SIZE 64
#define WIDE_BUFFER_SIZE 128

#define ENDSCREEN_WIDTH 44
#define ENDSCREEN_HEIGHT_OFFSET 8
#define ENDSCREEN_TITLE_Y_OFFSET 1
#define ENDSCREEN_WINNER_Y_OFFSET 2
#define ENDSCREEN_TABLE_Y_OFFSET 4
#define ENDSCREEN_PROMPT_Y_OFFSET 2
#define ENDSCREEN_TABLE_PADDING 12

static const int8_t NO_PLAYER = -1;

static const char *const PLAYER_FACES[] = {
    "[^_^]", "[o_O]", "[>_<]", "[-_-]", "[T_T]", "[=_=]", "[*_*]", "[@_@]", "[!_!]",
};

typedef struct {
    WINDOW *board_win;
    WINDOW *panel_win;
    int16_t term_rows;
    int16_t term_cols;
    int16_t board_rows;
    int16_t board_x_offset; // horizontal offset to center the board
    int16_t board_y_offset; // vertical offset to center the board
} view_t;

void view_init(view_t *view, uint16_t board_width, uint16_t board_height);
void view_cleanup(view_t *view);
void view_draw_board(view_t *view, game_state_t *state);
void view_draw_panels(view_t *view, game_state_t *state);
void view_draw_all(view_t *view, game_state_t *state);
void view_draw_endscreen(view_t *view, game_state_t *state);
