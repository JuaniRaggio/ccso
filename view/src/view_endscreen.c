#include "view.h"
#include "view_internal.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

static void draw_box(WINDOW *win, int16_t y, int16_t x, int16_t h, int16_t w) {
    mvwaddch(win, y, x, ACS_ULCORNER);
    for (int16_t i = 1; i < w - 1; i++)
        mvwaddch(win, y, x + i, ACS_HLINE);
    mvwaddch(win, y, x + w - 1, ACS_URCORNER);

    for (int16_t r = 1; r < h - 1; r++) {
        mvwaddch(win, y + r, x, ACS_VLINE);
        for (int16_t i = 1; i < w - 1; i++)
            mvwaddch(win, y + r, x + i, ' ');
        mvwaddch(win, y + r, x + w - 1, ACS_VLINE);
    }

    mvwaddch(win, y + h - 1, x, ACS_LLCORNER);
    for (int16_t i = 1; i < w - 1; i++)
        mvwaddch(win, y + h - 1, x + i, ACS_HLINE);
    mvwaddch(win, y + h - 1, x + w - 1, ACS_LRCORNER);
}

static void draw_centered_text(WINDOW *win, int16_t y, int16_t box_x, int16_t box_w, const char *text, int32_t attrs) {
    int16_t tx = box_x + (box_w - (int16_t)strlen(text)) / 2;
    wattron(win, attrs);
    mvwprintw(win, y, tx, "%s", text);
    wattroff(win, attrs);
}

static void draw_winner_banner(WINDOW *win, game_state_t *state, int8_t winner, int16_t y, int16_t box_x,
                               int16_t box_w) {
    wchar_t msg[WIDE_BUFFER_SIZE], ws_name[MAX_NAME_LENGTH + 1];
    mbstowcs(ws_name, display_name(state->players[winner].name), MAX_NAME_LENGTH);
    swprintf(msg, WIDE_BUFFER_SIZE, L"P%d: %ls wins!", winner, ws_name);

    int16_t wwidth = (int16_t)wcswidth(msg, wcslen(msg));
    int16_t wx = box_x + (box_w - wwidth) / 2;
    if (wx < box_x + 1)
        wx = box_x + 1;

    wattron(win, COLOR_PAIR(winner + COLOR_PAIR_OFFSET) | A_BOLD);
    mvwaddwstr(win, y, wx, msg);
    wattroff(win, COLOR_PAIR(winner + COLOR_PAIR_OFFSET) | A_BOLD);
}

static void draw_player_row(WINDOW *win, int16_t y, int16_t col_x, player_t *p, int8_t idx) {
    wchar_t ws_name[MAX_NAME_LENGTH + 1], ws_face[8];
    mbstowcs(ws_name, display_name(p->name), MAX_NAME_LENGTH);
    mbstowcs(ws_face, PLAYER_THEMES[idx % MAX_PLAYERS].flag, 7);
    int16_t nwidth = (int16_t)wcswidth(ws_name, wcslen(ws_name));

    wattron(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
    mvwprintw(win, y, col_x, " %d   ", idx);
    waddwstr(win, ws_face);
    wprintw(win, "    ");
    waddwstr(win, ws_name);
    int16_t pad = ENDSCREEN_TABLE_PADDING - nwidth;
    for (int16_t i = 0; i < pad; i++)
        waddch(win, ' ');
    wprintw(win, " %5u %5u %5u", p->score, p->valid_moves, p->invalid_moves);
    wattroff(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
}

static void draw_results_table(WINDOW *win, game_state_t *state, int8_t order[], int16_t y, int16_t col_x) {
    wattron(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    mvwprintw(win, y, col_x, " #  %-7s %-12s %5s %5s %5s", "Avatar", "Player", "Score", "Valid", "Inv");
    wattroff(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);

    for (int8_t pos = 0; pos < state->players_count; pos++) {
        int8_t idx = order[pos];
        draw_player_row(win, y + 1 + pos, col_x, &state->players[idx], idx);
    }
}

void view_draw_endscreen(view_t *view, game_state_t *state) {
    int8_t order[MAX_PLAYERS];
    sort_players_by_score(state, order);

    int16_t box_w = ENDSCREEN_WIDTH;
    int16_t box_h = (int16_t)(ENDSCREEN_HEIGHT_OFFSET + state->players_count);
    int16_t box_x = (view->term_cols - box_w) / 2;
    int16_t box_y = (view->board_rows - box_h) / 2;
    if (box_x < 0)
        box_x = 0;
    if (box_y < 0)
        box_y = 0;

    WINDOW *win = view->board_win;
    wattron(win, COLOR_PAIR(COLOR_BOARD));
    draw_box(win, box_y, box_x, box_h, box_w);
    wattroff(win, COLOR_PAIR(COLOR_BOARD));

    draw_centered_text(win, box_y + ENDSCREEN_TITLE_Y_OFFSET, box_x, box_w, "Game Over",
                       COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    draw_winner_banner(win, state, order[0], box_y + ENDSCREEN_WINNER_Y_OFFSET, box_x, box_w);
    draw_results_table(win, state, order, box_y + ENDSCREEN_TABLE_Y_OFFSET, box_x + 2);
    draw_centered_text(win, box_y + box_h - ENDSCREEN_PROMPT_Y_OFFSET, box_x, box_w, "Press any key to exit",
                       COLOR_PAIR(COLOR_BOARD));
    wrefresh(win);
}
