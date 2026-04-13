#include "view.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

static int8_t player_at(game_state_t *state, uint16_t col, uint16_t row) {
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].x == col && state->players[i].y == row)
            return i;
    }
    return NO_PLAYER;
}

static void draw_player_at(WINDOW *win, int16_t y, int16_t x, int8_t pidx) {
    const char *head = PLAYER_THEMES[pidx % MAX_PLAYERS].head;
    wchar_t ws[8];
    mbstowcs(ws, head, 7);

    wattron(win, COLOR_PAIR(pidx + COLOR_PAIR_OFFSET) | A_BOLD);
    mvwaddwstr(win, y, x, ws);
    wattroff(win, COLOR_PAIR(pidx + COLOR_PAIR_OFFSET) | A_BOLD);
}

static void draw_reward_at(WINDOW *win, int16_t y, int16_t x, int8_t value) {
    wattron(win, COLOR_PAIR(COLOR_BOARD));
    mvwprintw(win, y, x, "%2d", value);
    wattroff(win, COLOR_PAIR(COLOR_BOARD));
}

static void draw_trail_at(WINDOW *win, int16_t y, int16_t x, int8_t value) {
    int8_t eater = (int8_t)(-value);
    const char *trail = PLAYER_THEMES[eater % MAX_PLAYERS].flag;
    wchar_t ws[8];
    mbstowcs(ws, trail, 7);

    wattron(win, COLOR_PAIR(eater + COLOR_PAIR_OFFSET) | A_DIM);
    mvwaddwstr(win, y, x, ws);
    wattroff(win, COLOR_PAIR(eater + COLOR_PAIR_OFFSET) | A_DIM);
}

static void draw_cell(WINDOW *win, game_state_t *state, int16_t y, int16_t x, uint16_t col, uint16_t row) {
    int8_t value = state->board[row * state->width + col];
    int8_t pidx = player_at(state, col, row);

    if (pidx != NO_PLAYER)
        draw_player_at(win, y, x, pidx);
    else if (value > 0)
        draw_reward_at(win, y, x, value);
    else
        draw_trail_at(win, y, x, value);
}

static void draw_stadium_ring(WINDOW *win, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t color) {
    wattron(win, COLOR_PAIR(color + COLOR_PAIR_OFFSET) | A_BOLD);

    for (int16_t x = x1; x <= x2; x++) {
        mvwaddch(win, y1, x, ACS_CKBOARD);
        mvwaddch(win, y2, x, ACS_CKBOARD);
    }
    for (int16_t y = y1; y <= y2; y++) {
        mvwaddch(win, y, x1, ACS_CKBOARD);
        mvwaddch(win, y, x1 + 1, ACS_CKBOARD);
        mvwaddch(win, y, x2, ACS_CKBOARD);
        mvwaddch(win, y, x2 + 1, ACS_CKBOARD);
    }

    wattroff(win, COLOR_PAIR(color + COLOR_PAIR_OFFSET) | A_BOLD);
}

static void draw_stadium_title(WINDOW *win, int16_t x1, int16_t x2, int16_t y, int16_t color) {
    const char *title = "  CHOMP CHAMPS WORLD CUP  ";
    int16_t tx = x1 + (x2 - x1 - (int16_t)strlen(title)) / 2;
    wattron(win, COLOR_PAIR(color + COLOR_PAIR_OFFSET) | A_BOLD);
    mvwprintw(win, y, tx, "%s", title);
    wattroff(win, COLOR_PAIR(color + COLOR_PAIR_OFFSET) | A_BOLD);
}

static void draw_stadium_border(view_t *view, uint16_t width, uint16_t height) {
    int16_t colors[] = {COLOR_CYAN, COLOR_MAGENTA, COLOR_BLUE, COLOR_YELLOW};
    int16_t max_h = view->board_x_offset / 3;
    int16_t max_v = view->board_y_offset / 2;
    int16_t rings = (max_h < max_v) ? max_h : max_v;
    if (rings > 4) rings = 4;
    if (rings < 1) rings = 1;

    for (int16_t r = rings - 1; r >= 0; r--) {
        int16_t x1 = view->board_x_offset - 2 - r * 2;
        int16_t y1 = view->board_y_offset - 1 - r;
        int16_t x2 = view->board_x_offset + width * CELL_WIDTH + r * 2;
        int16_t y2 = view->board_y_offset + height + r;
        int16_t color = colors[(view->frame_count / 2 + r) % 4];

        draw_stadium_ring(view->board_win, x1, y1, x2, y2, color);
        if (r == 0)
            draw_stadium_title(view->board_win, x1, x2, y1, color);
    }
}

void view_draw_board(view_t *view, game_state_t *state) {
    werase(view->board_win);
    draw_stadium_border(view, state->width, state->height);

    for (uint16_t row = 0; row < state->height; row++) {
        int16_t y = view->board_y_offset + (int16_t)row;
        if (y < 0 || y >= view->board_rows)
            continue;
        for (uint16_t col = 0; col < state->width; col++) {
            int16_t x = view->board_x_offset + col * CELL_WIDTH;
            if (x < 0 || x + CELL_WIDTH > view->term_cols)
                continue;
            draw_cell(view->board_win, state, y, x, col, row);
            if (col < state->width - 1)
                mvwaddch(view->board_win, y, x + 2, ' ');
        }
    }
    wrefresh(view->board_win);
}
