#define _GNU_SOURCE
#include "view.h"
#include "view_internal.h"
#include <game_state.h>
#include <game_sync.h>
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

const char *display_name(const char *name) {
    if (strncmp(name, PLAYER_PREFIX, PLAYER_PREFIX_LEN) == 0)
        return name + PLAYER_PREFIX_LEN;
    return name;
}

void sort_players_by_score(game_state_t *state, int8_t order[]) {
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

static void init_player_colors() {
    int16_t player_colors[MAX_PLAYERS] = {
        COLOR_RED,  COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,  COLOR_MAGENTA,
        COLOR_CYAN, COLOR_WHITE, COLOR_RED,    COLOR_GREEN,
    };
    for (int16_t i = 0; i < MAX_PLAYERS; i++)
        init_pair(i + COLOR_PAIR_OFFSET, player_colors[i], COLOR_BLACK);
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

static void view_draw_all(view_t *view, game_state_t *state) {
    view->frame_count++;
    view_draw_board(view, state);
    view_draw_panels(view, state);
}

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
