#define _GNU_SOURCE
#include "view.h"
#include <game_state.h>
#include <game_sync.h>
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// ============================================================
//  Signal handling
// ============================================================

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

// ============================================================
//  Helpers
// ============================================================

static int8_t player_at(game_state_t *state, uint16_t col, uint16_t row) {
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].x == col && state->players[i].y == row)
            return i;
    }
    return NO_PLAYER;
}

static const char *display_name(const char *name) {
    if (strncmp(name, PLAYER_PREFIX, PLAYER_PREFIX_LEN) == 0)
        return name + PLAYER_PREFIX_LEN;
    return name;
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

// ============================================================
//  Ncurses setup
// ============================================================

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
    if (view->board_rows < 1)
        view->board_rows = 1;

    int16_t board_char_width = (int16_t)(board_width * CELL_WIDTH);
    view->board_x_offset = (view->term_cols - board_char_width) / 2;
    if (view->board_x_offset < 0)
        view->board_x_offset = 0;

    view->board_y_offset = (view->board_rows - (int16_t)board_height) / 2;
    if (view->board_y_offset < 0)
        view->board_y_offset = 0;
}

static void setup_ncurses() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();
}

static void create_windows(view_t *view, uint16_t width, uint16_t height) {
    init_player_colors();
    getmaxyx(stdscr, view->term_rows, view->term_cols);
    calculate_board_layout(view, width, height);
    view->board_win = newwin(view->board_rows, view->term_cols, 0, 0);
    view->panel_win = newwin(PANEL_HEIGHT, view->term_cols, view->board_rows, 0);
}

// ============================================================
//  Lifecycle
// ============================================================

void view_init(view_t *view, uint16_t board_width, uint16_t board_height) {
    if (!setlocale(LC_ALL, "C.utf8"))
        setlocale(LC_ALL, "");
    if (getenv("TERM") == NULL)
        setenv("TERM", "xterm-256color", 0);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    setup_ncurses();
    view->frame_count = 0;
    create_windows(view, board_width, board_height);
}

void view_cleanup(view_t *view) {
    if (view->board_win != NULL)
        delwin(view->board_win);
    if (view->panel_win != NULL)
        delwin(view->panel_win);
    endwin();
}

static void view_handle_resize(view_t *view, uint16_t width, uint16_t height) {
    view_cleanup(view);
    setup_ncurses();
    create_windows(view, width, height);
}

// ============================================================
//  Board cell drawing
// ============================================================

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

// ============================================================
//  Stadium border
// ============================================================

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

static int16_t compute_ring_count(view_t *view) {
    int16_t max_h = view->board_x_offset / 3;
    int16_t max_v = view->board_y_offset / 2;
    int16_t rings = (max_h < max_v) ? max_h : max_v;
    if (rings > 4) rings = 4;
    if (rings < 1) rings = 1;
    return rings;
}

static void draw_stadium_border(view_t *view, uint16_t width, uint16_t height) {
    int16_t colors[] = {COLOR_CYAN, COLOR_MAGENTA, COLOR_BLUE, COLOR_YELLOW};
    int16_t rings = compute_ring_count(view);

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

// ============================================================
//  Board
// ============================================================

static void view_draw_board(view_t *view, game_state_t *state) {
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

// ============================================================
//  Player panels
// ============================================================

static void draw_panel_border(WINDOW *win, int16_t y, int16_t x, int16_t w) {
    mvwaddch(win, y, x, '+');
    for (int16_t i = 1; i < w - 1; i++)
        mvwaddch(win, y, x + i, '=');
    mvwaddch(win, y, x + w - 1, '+');
}

static void draw_panel_header(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx) {
    const char *name = display_name(player->name);
    const char *status = player->state ? "ALIVE" : "DEAD";
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

    if (!player->state)
        wattron(win, A_DIM);
    mvwaddwstr(win, PLAYER_PANEL_Y_OFFSET, hstart, header);
    if (!player->state)
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
    mvwprintw(win, row, x + 2, "(%u,%u)  V:%-4u I:%-4u", player->x, player->y,
              player->valid_moves, player->invalid_moves);
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

static void view_draw_panels(view_t *view, game_state_t *state) {
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

static void view_draw_all(view_t *view, game_state_t *state) {
    view->frame_count++;
    view_draw_board(view, state);
    view_draw_panels(view, state);
}

// ============================================================
//  Endscreen
// ============================================================

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

static void draw_endscreen_winner(WINDOW *win, game_state_t *state, int8_t winner, int16_t y, int16_t box_x,
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

static void draw_endscreen_player_row(WINDOW *win, int16_t y, int16_t col_x, player_t *p, int8_t idx) {
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

static void draw_endscreen_table(WINDOW *win, game_state_t *state, int8_t order[], int16_t y, int16_t col_x) {
    wattron(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    mvwprintw(win, y, col_x, " #  %-7s %-12s %5s %5s %5s", "Avatar", "Player", "Score", "Valid", "Inv");
    wattroff(win, COLOR_PAIR(COLOR_BOARD) | A_BOLD);

    for (int8_t pos = 0; pos < state->players_count; pos++) {
        int8_t idx = order[pos];
        draw_endscreen_player_row(win, y + 1 + pos, col_x, &state->players[idx], idx);
    }
}

static void view_draw_endscreen(view_t *view, game_state_t *state) {
    int8_t order[MAX_PLAYERS];
    sort_players_by_score(state, order);

    int16_t box_w = ENDSCREEN_WIDTH;
    int16_t box_h = (int16_t)(ENDSCREEN_HEIGHT_OFFSET + state->players_count);
    int16_t box_x = (view->term_cols - box_w) / 2;
    int16_t box_y = (view->board_rows - box_h) / 2;
    if (box_x < 0) box_x = 0;
    if (box_y < 0) box_y = 0;

    WINDOW *win = view->board_win;
    wattron(win, COLOR_PAIR(COLOR_BOARD));
    draw_box(win, box_y, box_x, box_h, box_w);
    wattroff(win, COLOR_PAIR(COLOR_BOARD));

    draw_centered_text(win, box_y + ENDSCREEN_TITLE_Y_OFFSET, box_x, box_w, "Game Over",
                       COLOR_PAIR(COLOR_BOARD) | A_BOLD);
    draw_endscreen_winner(win, state, order[0], box_y + ENDSCREEN_WINNER_Y_OFFSET, box_x, box_w);
    draw_endscreen_table(win, state, order, box_y + ENDSCREEN_TABLE_Y_OFFSET, box_x + 2);
    draw_centered_text(win, box_y + box_h - ENDSCREEN_PROMPT_Y_OFFSET, box_x, box_w, "Press any key to exit",
                       COLOR_PAIR(COLOR_BOARD));
    wrefresh(win);
}

// ============================================================
//  Public API
// ============================================================

void view_run(view_t *view, game_t *game, uint16_t width, uint16_t height) {
    while (1) {
        game_sync_view_wait_frame(game->sync);
        if (should_exit || !game->state->running) {
            game_sync_view_frame_done(game->sync);
            break;
        }

        if (getch() == KEY_RESIZE)
            view_handle_resize(view, width, height);

        view_draw_all(view, game->state);
        game_sync_view_frame_done(game->sync);
    }
}

void view_show_results(view_t *view, game_state_t *state) {
    if (should_exit)
        return;
    view_draw_all(view, state);
    view_draw_endscreen(view, state);
    nodelay(stdscr, FALSE);
    getch();
}
