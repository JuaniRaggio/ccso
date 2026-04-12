#include "view.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>

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

    int16_t board_h = view->term_rows - PANEL_BOTTOM_HEIGHT;
    if (board_h < 1) {
        board_h = 1;
    }

    view->board_win = newwin(board_h, view->term_cols, 0, 0);
    view->panel_win = newwin(PANEL_BOTTOM_HEIGHT, view->term_cols, board_h, 0);
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

static void draw_hex_cell(WINDOW *win, int16_t col, int16_t row, int8_t value, int8_t player_idx) {
    int16_t sx = col * (HEX_CELL_WIDTH - HEX_CELL_OVERLAP);

    int16_t sy = row * HEX_CELL_BASE_HEIGHT;
    if (col % 2 != 0) {
        sy += HEX_CELL_BASE_HEIGHT / HEX_STAGGER_DIVISOR;
    }

    int16_t color_pair;
    if (player_idx != NO_PLAYER) {
        color_pair = player_idx + COLOR_PAIR_OFFSET;
    } else if (value <= 0) {
        // Eaten cell: value = -player_id, so eater index = -value
        int8_t eater = (int8_t)(-value);
        color_pair = eater + COLOR_PAIR_OFFSET;
    } else {
        color_pair = COLOR_BOARD;
    }

    wattron(win, COLOR_PAIR(color_pair));

    if (player_idx != NO_PLAYER) {
        // Player standing here: bold hex with player marker
        wattron(win, A_BOLD);
        mvwprintw(win, sy + HEX_TOP_LINE, sx, "/---\\");
        mvwprintw(win, sy + HEX_BODY_LINE, sx, "|P%-2d|", player_idx);
        mvwprintw(win, sy + HEX_BOTTOM_LINE, sx, "\\---/");
        wattroff(win, A_BOLD);
    } else if (value > 0) {
        // Unclaimed cell with value
        mvwprintw(win, sy + HEX_TOP_LINE, sx, "/---\\");
        mvwprintw(win, sy + HEX_BODY_LINE, sx, "| %d |", value);
        mvwprintw(win, sy + HEX_BOTTOM_LINE, sx, "\\---/");
    } else {
        // Eaten cell: dim colored marker
        mvwprintw(win, sy + HEX_TOP_LINE, sx, "/---\\");
        mvwprintw(win, sy + HEX_BODY_LINE, sx, "| . |");
        mvwprintw(win, sy + HEX_BOTTOM_LINE, sx, "\\---/");
    }

    wattroff(win, COLOR_PAIR(color_pair));
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

static void draw_player_panel(WINDOW *win, int16_t x_offset, int16_t panel_w, player_t *player, int8_t idx) {
    int16_t color = idx + COLOR_PAIR_OFFSET;

    wattron(win, COLOR_PAIR(color));

    // Top border with label
    mvwaddch(win, 0, x_offset, '+');
    for (int16_t i = 1; i < panel_w - 1; i++) {
        mvwaddch(win, 0, x_offset + i, '-');
    }
    mvwaddch(win, 0, x_offset + panel_w - 1, '+');

    char label[LABEL_BUFFER_SIZE];
    snprintf(label, sizeof(label), " %s P%d: %.8s ", PLAYER_FACES[idx % MAX_PLAYERS], idx, player->name);
    int16_t label_len = (int16_t)strlen(label);
    int16_t label_start = x_offset + (panel_w - label_len) / 2;
    if (label_start < x_offset + 1) {
        label_start = x_offset + 1;
    }
    mvwprintw(win, 0, label_start, "%s", label);

    // Stats line
    mvwaddch(win, 1, x_offset, '|');
    char stats[64];
    snprintf(stats, sizeof(stats), " S:%-5u V:%-4u I:%-4u", player->score, player->valid_moves, player->invalid_moves);
    int16_t stats_len = (int16_t)strlen(stats);
    mvwprintw(win, 1, x_offset + 1, "%s", stats);
    for (int16_t i = stats_len + 1; i < panel_w - 1; i++) {
        mvwaddch(win, 1, x_offset + i, ' ');
    }
    mvwaddch(win, 1, x_offset + panel_w - 1, '|');

    // Bottom border
    mvwaddch(win, 2, x_offset, '+');
    for (int16_t i = 1; i < panel_w - 1; i++) {
        mvwaddch(win, 2, x_offset + i, '-');
    }
    mvwaddch(win, 2, x_offset + panel_w - 1, '+');

    wattroff(win, COLOR_PAIR(color));
}

void view_draw_panels(view_t *view, game_state_t *state) {
    werase(view->panel_win);

    if (state->players_count <= 0) {
        wrefresh(view->panel_win);
        return;
    }

    int16_t panel_w = view->term_cols / state->players_count;
    if (panel_w < 10) {
        panel_w = 10;
    }

    for (int8_t i = 0; i < state->players_count; i++) {
        int16_t x_offset = i * panel_w;
        int16_t w = (i == state->players_count - 1) ? (view->term_cols - x_offset) : panel_w;
        draw_player_panel(view->panel_win, x_offset, w, &state->players[i], i);
    }

    wrefresh(view->panel_win);
}

void view_draw_all(view_t *view, game_state_t *state) {
    view_draw_board(view, state);
    view_draw_panels(view, state);
}
