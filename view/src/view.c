#define _GNU_SOURCE
#include "view.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

static int8_t player_at(game_state_t *state, uint16_t col, uint16_t row) {
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].x == col && state->players[i].y == row) {
            return i;
        }
    }
    return NO_PLAYER;
}

static const char *display_name(const char *name) {
    if (strncmp(name, PLAYER_PREFIX, PLAYER_PREFIX_LEN) == 0) {
        return name + PLAYER_PREFIX_LEN;
    }
    return name;
}

static void init_player_colors() {
    int16_t player_colors[MAX_PLAYERS] = {
        COLOR_RED,  COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,  COLOR_MAGENTA,
        COLOR_CYAN, COLOR_WHITE, COLOR_RED,    COLOR_GREEN,
    };
    for (int16_t i = 0; i < MAX_PLAYERS; i++) {
        init_pair(i + COLOR_PAIR_OFFSET, player_colors[i], COLOR_BLACK);
    }
    init_pair(COLOR_BOARD, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_EMPTY, COLOR_BLACK, COLOR_BLACK);
}

static void calculate_board_layout(view_t *view, uint16_t board_width, uint16_t board_height) {
    view->board_rows = view->term_rows - PANEL_HEIGHT;
    if (view->board_rows < 1) {
        view->board_rows = 1;
    }

    int16_t board_char_width = (int16_t)(board_width * CELL_WIDTH);
    view->board_x_offset = (view->term_cols - board_char_width) / 2;
    if (view->board_x_offset < 0) {
        view->board_x_offset = 0;
    }

    view->board_y_offset = (view->board_rows - (int16_t)board_height) / 2;
    if (view->board_y_offset < 0) {
        view->board_y_offset = 0;
    }
}

void view_init(view_t *view, uint16_t board_width, uint16_t board_height) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();

    init_player_colors();
    getmaxyx(stdscr, view->term_rows, view->term_cols);
    calculate_board_layout(view, board_width, board_height);

    view->board_win = newwin(view->board_rows, view->term_cols, 0, 0);
    view->panel_win = newwin(PANEL_HEIGHT, view->term_cols, view->board_rows, 0);
}

void view_cleanup(view_t *view) {
    if (view->board_win != NULL)
        delwin(view->board_win);
    if (view->panel_win != NULL)
        delwin(view->panel_win);
    endwin();
}

static void draw_player_at(WINDOW *win, int16_t y, int16_t x, int8_t pidx) {
    wattron(win, COLOR_PAIR(pidx + COLOR_PAIR_OFFSET) | A_BOLD);
    mvwprintw(win, y, x, "P%d", pidx);
    wattroff(win, COLOR_PAIR(pidx + COLOR_PAIR_OFFSET) | A_BOLD);
}

static void draw_reward_at(WINDOW *win, int16_t y, int16_t x, int8_t value) {
    wattron(win, COLOR_PAIR(COLOR_BOARD));
    mvwprintw(win, y, x, "%2d", value);
    wattroff(win, COLOR_PAIR(COLOR_BOARD));
}

static void draw_trail_at(WINDOW *win, int16_t y, int16_t x, int8_t value) {
    int8_t eater = (int8_t)(-value);
    wattron(win, COLOR_PAIR(eater + COLOR_PAIR_OFFSET) | A_DIM);
    mvwprintw(win, y, x, " *");
    wattroff(win, COLOR_PAIR(eater + COLOR_PAIR_OFFSET) | A_DIM);
}

void view_draw_board(view_t *view, game_state_t *state) {
    werase(view->board_win);

    for (uint16_t row = 0; row < state->height; row++) {
        int16_t y = view->board_y_offset + (int16_t)row;
        if (y < 0 || y >= view->board_rows)
            continue;

        for (uint16_t col = 0; col < state->width; col++) {
            int16_t x = view->board_x_offset + col * CELL_WIDTH;
            if (x < 0 || x + CELL_WIDTH > view->term_cols)
                continue;

            int8_t value = state->board[row * state->width + col];
            int8_t pidx = player_at(state, col, row);

            if (pidx != NO_PLAYER) {
                draw_player_at(view->board_win, y, x, pidx);
            } else if (value > 0) {
                draw_reward_at(view->board_win, y, x, value);
            } else {
                draw_trail_at(view->board_win, y, x, value);
            }

            if (col < state->width - 1) {
                mvwaddch(view->board_win, y, x + 2, ' ');
            }
        }
    }
    wrefresh(view->board_win);
}

static void draw_panel_border(WINDOW *win, int16_t y, int16_t x, int16_t w) {
    mvwaddch(win, y, x, '+');
    for (int16_t i = 1; i < w - 1; i++) {
        mvwaddch(win, y, x + i, '=');
    }
    mvwaddch(win, y, x + w - 1, '+');
}

static void draw_player_panel(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx, int8_t rank) {
    int16_t color = idx + COLOR_PAIR_OFFSET;
    const char *name = display_name(player->name);
    const char *face = PLAYER_FACES[idx % MAX_PLAYERS];
    const char *status = player->state ? "ALIVE" : "DEAD";

    wchar_t ws_name[MAX_NAME_LENGTH + 1], ws_status[STATUS_BUFFER_SIZE];
    mbstowcs(ws_name, name, MAX_NAME_LENGTH);
    mbstowcs(ws_status, status, STATUS_BUFFER_SIZE - 1);

    wattron(win, COLOR_PAIR(color));
    draw_panel_border(win, PLAYER_PANEL_Y_OFFSET + 0, x, w);

    wchar_t header[WIDE_BUFFER_SIZE];
    swprintf(header, WIDE_BUFFER_SIZE, L" %ls [%ls] ", ws_name, ws_status);
    int16_t hwidth = (int16_t)wcswidth(header, wcslen(header));
    int16_t hstart = x + (w - hwidth) / 2;
    if (hstart < x + 1)
        hstart = x + 1;

    if (!player->state)
        wattron(win, A_DIM);
    mvwaddwstr(win, PLAYER_PANEL_Y_OFFSET + 0, hstart, header);
    if (!player->state)
        wattroff(win, A_DIM);

    char line1[LINE_BUFFER_SIZE], line2[LINE_BUFFER_SIZE];
    snprintf(line1, LINE_BUFFER_SIZE, " %s  Score: %-5u", face, player->score);
    snprintf(line2, LINE_BUFFER_SIZE, " (%u,%u)  V:%-4u I:%-4u", player->x, player->y, player->valid_moves,
             player->invalid_moves);

    mvwaddch(win, PLAYER_PANEL_Y_OFFSET + 1, x, '|');
    mvwprintw(win, PLAYER_PANEL_Y_OFFSET + 1, x + 1, "%-*s", w - 2, line1);
    mvwaddch(win, PLAYER_PANEL_Y_OFFSET + 1, x + w - 1, '|');

    mvwaddch(win, PLAYER_PANEL_Y_OFFSET + 2, x, '|');
    mvwprintw(win, PLAYER_PANEL_Y_OFFSET + 2, x + 1, "%-*s", w - 2, line2);
    mvwaddch(win, PLAYER_PANEL_Y_OFFSET + 2, x + w - 1, '|');

    draw_panel_border(win, PLAYER_PANEL_Y_OFFSET + 3, x, w);
    wattroff(win, COLOR_PAIR(color));
}

static void sort_players_by_score(game_state_t *state, int8_t order[]) {
    for (int8_t i = 0; i < state->players_count; i++)
        order[i] = i;
    for (int8_t i = 0; i < state->players_count - 1; i++) {
        for (int8_t j = i + 1; j < state->players_count; j++) {
            if (state->players[order[j]].score > state->players[order[i]].score) {
                int8_t tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
}

void view_draw_panels(view_t *view, game_state_t *state) {
    werase(view->panel_win);
    if (state->players_count <= 0) {
        wrefresh(view->panel_win);
        return;
    }

    const char *label = "--- LEADERBOARD ---";
    int16_t lx = (view->term_cols - (int16_t)strlen(label)) / 2;
    if (lx < 0)
        lx = 0;
    wattron(view->panel_win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    mvwprintw(view->panel_win, LEADERBOARD_LABEL_Y, lx, "%s", label);
    wattroff(view->panel_win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);

    int8_t order[MAX_PLAYERS];
    sort_players_by_score(state, order);

    int16_t panel_w = view->term_cols / state->players_count;
    if (panel_w < MIN_PANEL_WIDTH)
        panel_w = MIN_PANEL_WIDTH;

    for (int8_t pos = 0; pos < state->players_count; pos++) {
        int8_t idx = order[pos];
        int16_t x_offset = pos * panel_w;
        int16_t w = (pos == state->players_count - 1) ? (view->term_cols - x_offset) : panel_w;
        draw_player_panel(view->panel_win, x_offset, w, &state->players[idx], idx, pos + 1);
    }
    wrefresh(view->panel_win);
}

void view_draw_all(view_t *view, game_state_t *state) {
    view_draw_board(view, state);
    view_draw_panels(view, state);
}

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

void view_draw_endscreen(view_t *view, game_state_t *state) {
    int8_t order[MAX_PLAYERS];
    sort_players_by_score(state, order);
    int8_t winner = order[0];

    int16_t box_w = ENDSCREEN_WIDTH, box_h = (int16_t)(ENDSCREEN_HEIGHT_OFFSET + state->players_count);
    int16_t box_x = (view->term_cols - box_w) / 2, box_y = (view->board_rows - box_h) / 2;
    if (box_x < 0)
        box_x = 0;
    if (box_y < 0)
        box_y = 0;

    WINDOW *win = view->board_win;
    wattron(win, COLOR_PAIR(COLOR_BOARD));
    draw_box(win, box_y, box_x, box_h, box_w);
    wattroff(win, COLOR_PAIR(COLOR_BOARD));

    const char *title = "Game Over";
    int16_t tx = box_x + (box_w - (int16_t)strlen(title)) / 2;
    wattron(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    mvwprintw(win, box_y + ENDSCREEN_TITLE_Y_OFFSET, tx, "%s", title);
    wattroff(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);

    wchar_t winner_msg[WIDE_BUFFER_SIZE];
    wchar_t ws_wname[MAX_NAME_LENGTH + 1];
    mbstowcs(ws_wname, display_name(state->players[winner].name), MAX_NAME_LENGTH);
    swprintf(winner_msg, WIDE_BUFFER_SIZE, L"P%d: %ls wins!", winner, ws_wname);
    int16_t wwidth = (int16_t)wcswidth(winner_msg, wcslen(winner_msg));
    int16_t wx = box_x + (box_w - wwidth) / 2;
    if (wx < box_x + 1)
        wx = box_x + 1;
    wattron(win, COLOR_PAIR(winner + COLOR_PAIR_OFFSET) | A_BOLD);
    mvwaddwstr(win, box_y + ENDSCREEN_WINNER_Y_OFFSET, wx, winner_msg);
    wattroff(win, COLOR_PAIR(winner + COLOR_PAIR_OFFSET) | A_BOLD);

    int16_t col_x = box_x + 2;
    wattron(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    mvwprintw(win, box_y + ENDSCREEN_TABLE_Y_OFFSET, col_x, " #  %-12s %5s %5s %5s", "Player", "Score", "Valid", "Inv");
    wattroff(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);

    for (int8_t pos = 0; pos < state->players_count; pos++) {
        int8_t idx = order[pos];
        player_t *p = &state->players[idx];
        wchar_t ws_name[MAX_NAME_LENGTH + 1];
        mbstowcs(ws_name, display_name(p->name), MAX_NAME_LENGTH);
        int16_t nwidth = (int16_t)wcswidth(ws_name, wcslen(ws_name));
        wattron(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
        mvwaddwstr(win, box_y + ENDSCREEN_TABLE_Y_OFFSET + 1 + pos, col_x, L" ");
        wprintw(win, "%d  ", idx);
        waddwstr(win, ws_name);
        int16_t pad = ENDSCREEN_TABLE_PADDING - nwidth;
        if (pad < 0)
            pad = 0;
        for (int16_t i = 0; i < pad; i++)
            waddch(win, ' ');
        wprintw(win, " %5u %5u %5u", p->score, p->valid_moves, p->invalid_moves);
        wattroff(win, COLOR_PAIR(idx + COLOR_PAIR_OFFSET));
    }

    const char *prompt = "Press any key to exit";
    int16_t px = box_x + (box_w - (int16_t)strlen(prompt)) / 2;
    wattron(win, COLOR_PAIR(COLOR_BOARD));
    mvwprintw(win, box_y + box_h - ENDSCREEN_PROMPT_Y_OFFSET, px, "%s", prompt);
    wattroff(win, COLOR_PAIR(COLOR_BOARD));
    wrefresh(win);
}
