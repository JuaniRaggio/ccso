#include "view.h"
#include "view_internal.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

static void draw_panel_border(WINDOW *win, int16_t y, int16_t x, int16_t w) {
    mvwaddch(win, y, x, '+');
    for (int16_t i = 1; i < w - 1; i++)
        mvwaddch(win, y, x + i, '=');
    mvwaddch(win, y, x + w - 1, '+');
}

static void draw_panel_header(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx) {
    const char *name = display_name(player->name);
    const char *status = player->is_blocked ? "DEAD" : "ALIVE";
    wchar_t ws_name[MAX_NAME_LENGTH + 1], ws_status[STATUS_BUFFER_SIZE];
    mbstowcs(ws_name, name, MAX_NAME_LENGTH);
    mbstowcs(ws_status, status, STATUS_BUFFER_SIZE - 1);

    wchar_t header[WIDE_BUFFER_SIZE];
    swprintf(header, WIDE_BUFFER_SIZE, L" %ls [%ls] ", ws_name, ws_status);
    int16_t hwidth = (int16_t)wcswidth(header, wcslen(header));
    int16_t hstart = x + (w - hwidth) / 2;
    if (hstart < x + 1)
        hstart = x + 1;

    wattron(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
    draw_panel_border(win, PLAYER_PANEL_Y_OFFSET, x, w);

    if (player->is_blocked)
        wattron(win, A_DIM);
    mvwaddwstr(win, PLAYER_PANEL_Y_OFFSET, hstart, header);
    if (player->is_blocked)
        wattroff(win, A_DIM);

    wattroff(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
}

static void draw_panel_score_row(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx) {
    wchar_t ws_face[8];
    mbstowcs(ws_face, PLAYER_THEMES[idx % MAX_PLAYERS].flag, 7);

    int16_t row = PLAYER_PANEL_Y_OFFSET + 1;
    wattron(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
    mvwaddch(win, row, x, '|');
    mvwaddwstr(win, row, x + 3, ws_face);
    mvwprintw(win, row, x + 6, "Score: %-5u", player->score);
    mvwaddch(win, row, x + w - 1, '|');
    wattroff(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
}

static void draw_panel_stats_row(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx) {
    int16_t row = PLAYER_PANEL_Y_OFFSET + 2;
    wattron(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
    mvwaddch(win, row, x, '|');
    mvwprintw(win, row, x + 2, "(%u,%u)  V:%-4u I:%-4u", player->x, player->y, player->valid_moves,
              player->invalid_moves);
    mvwaddch(win, row, x + w - 1, '|');
    wattroff(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
}

static void draw_player_panel(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx) {
    draw_panel_header(win, x, w, player, idx);
    draw_panel_score_row(win, x, w, player, idx);
    draw_panel_stats_row(win, x, w, player, idx);
    wattron(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
    draw_panel_border(win, PLAYER_PANEL_Y_OFFSET + 3, x, w);
    wattroff(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
}

static void draw_leaderboard_label(WINDOW *win, int16_t term_cols) {
    const char *label = "--- LEADERBOARD ---";
    int16_t lx = (term_cols - (int16_t)strlen(label)) / 2;
    if (lx < 0)
        lx = 0;
    wattron(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    mvwprintw(win, LEADERBOARD_LABEL_Y, lx, "%s", label);
    wattroff(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
}

void view_draw_panels(view_t *view, game_state_t *state) {
    werase(view->panel_win);
    if (state->players_count <= 0) {
        wrefresh(view->panel_win);
        return;
    }

    draw_leaderboard_label(view->panel_win, view->term_cols);

    int8_t order[MAX_PLAYERS];
    sort_players_by_score(state, order);

    int16_t panel_w = view->term_cols / state->players_count;
    if (panel_w < MIN_PANEL_WIDTH)
        panel_w = MIN_PANEL_WIDTH;

    for (int8_t pos = 0; pos < state->players_count; pos++) {
        int8_t idx = order[pos];
        int16_t x_offset = pos * panel_w;
        int16_t w = (pos == state->players_count - 1) ? (view->term_cols - x_offset) : panel_w;
        draw_player_panel(view->panel_win, x_offset, w, &state->players[idx], idx);
    }
    wrefresh(view->panel_win);
}
