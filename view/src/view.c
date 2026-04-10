#include "view.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>

static inline int16_t elevation(int8_t cell_value) {
    if (cell_value < MIN_CELL_VALUE || cell_value > MAX_CELL_VALUE) {
        return 0;
    }
    return (int16_t)((cell_value - MIN_CELL_VALUE) * HEX_MAX_ELEVATION / CELL_VALUE_RANGE);
}

static int8_t player_at(game_state_t *state, uint16_t col, uint16_t row) {
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].x == col && state->players[i].y == row) {
            return i;
        }
    }
    return NO_PLAYER;
}

void view_init(view_t *view) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();

    int16_t player_colors[MAX_PLAYERS] = {
        COLOR_RED,  COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,  COLOR_MAGENTA,
        COLOR_CYAN, COLOR_WHITE, COLOR_RED,    COLOR_GREEN,
    };
    for (int16_t i = 0; i < MAX_PLAYERS; i++) {
        init_pair(i + COLOR_PAIR_OFFSET, player_colors[i], COLOR_BLACK);
    }
    init_pair(COLOR_BOARD, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_EMPTY, COLOR_BLACK, COLOR_BLACK);

    getmaxyx(stdscr, view->term_rows, view->term_cols);

    int16_t board_width = view->term_cols - PANEL_WIDTH - 1;
    if (board_width < MIN_BOARD_WIDTH) {
        board_width = view->term_cols;
    }

    view->board_win = newwin(view->term_rows, board_width, 0, 0);
    view->panel_win = newwin(view->term_rows, PANEL_WIDTH, 0, board_width + 1);
}

void view_cleanup(view_t *view) {
    if (view->board_win != NULL) {
        delwin(view->board_win);
    }
    if (view->panel_win != NULL) {
        delwin(view->panel_win);
    }
    endwin();
}

static void draw_hex_cell(WINDOW *win, int16_t col, int16_t row,
                          int8_t value, int8_t player_idx) {
}

void view_draw_board(view_t *view, game_state_t *state) {
    werase(view->board_win);

    for (uint16_t row = 0; row < state->height; row++) {
        for (uint16_t col = 0; col < state->width; col++) {
            int8_t value = state->board[row * state->width + col];
            int8_t pidx = player_at(state, col, row);
            draw_hex_cell(view->board_win, (int16_t)col, (int16_t)row, value, pidx);
        }
    }

    wrefresh(view->board_win);
}

static void draw_player_panel(WINDOW *win, int16_t panel_row, player_t *player, int8_t idx) {
    int16_t y = panel_row * PANEL_HEIGHT;
    int16_t color = idx + COLOR_PAIR_OFFSET;

    wattron(win, COLOR_PAIR(color));

    // Top border
    mvwprintw(win, y, 0, "+");
    for (int16_t i = 1; i < PANEL_WIDTH - 1; i++) {
        mvwaddch(win, y, i, '-');
    }
    mvwaddch(win, y, PANEL_WIDTH - 1, '+');

    // Player label centered on the top border
    char label[LABEL_BUFFER_SIZE];
    snprintf(label, sizeof(label), " P%d: %.10s ", idx, player->name);
    int16_t label_start = (PANEL_WIDTH - (int16_t)strlen(label)) / 2;
    mvwprintw(win, y, label_start, "%s", label);

    // Row 1: face + score
    mvwprintw(win, y + 1, 0, "| %s  Score: %-6u      |", PLAYER_FACES[idx % MAX_PLAYERS], player->score);

    // Row 2: current position on the board
    mvwprintw(win, y + 2, 0, "| Pos: (%-3u, %-3u)        |", player->x, player->y);

    // Row 3: valid/invalid move counts
    mvwprintw(win, y + 3, 0, "| Moves: %-5uv %-5ui   |", player->valid_moves, player->invalid_moves);

    // Bottom border
    mvwprintw(win, y + 4, 0, "+");
    for (int16_t i = 1; i < PANEL_WIDTH - 1; i++) {
        mvwaddch(win, y + 4, i, '-');
    }
    mvwaddch(win, y + 4, PANEL_WIDTH - 1, '+');

    wattroff(win, COLOR_PAIR(color));
}

void view_draw_panels(view_t *view, game_state_t *state) {
    werase(view->panel_win);

    for (int8_t i = 0; i < state->players_count; i++) {
        draw_player_panel(view->panel_win, i, &state->players[i], i);
    }

    wrefresh(view->panel_win);
}

void view_draw_all(view_t *view, game_state_t *state) {
    view_draw_board(view, state);
    view_draw_panels(view, state);
}
