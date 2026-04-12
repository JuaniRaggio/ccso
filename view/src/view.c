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

// Strip "player-" prefix from name for display
static const char *display_name(const char *name) {
    if (strncmp(name, PLAYER_PREFIX, PLAYER_PREFIX_LEN) == 0) {
        return name + PLAYER_PREFIX_LEN;
    }
    return name;
}

void view_init(view_t *view, uint16_t board_width, uint16_t board_height) {
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

    view->board_rows = view->term_rows - PANEL_HEIGHT;
    if (view->board_rows < 1) {
        view->board_rows = 1;
    }

    // Center the board horizontally
    int16_t board_char_width = (int16_t)(board_width * CELL_WIDTH);
    view->board_x_offset = (view->term_cols - board_char_width) / 2;
    if (view->board_x_offset < 0) {
        view->board_x_offset = 0;
    }

    // Center the board vertically within the board window
    view->board_y_offset = (view->board_rows - (int16_t)board_height) / 2;
    if (view->board_y_offset < 0) {
        view->board_y_offset = 0;
    }

    view->board_win = newwin(view->board_rows, view->term_cols, 0, 0);
    view->panel_win = newwin(PANEL_HEIGHT, view->term_cols, view->board_rows, 0);
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

void view_draw_board(view_t *view, game_state_t *state) {
    werase(view->board_win);

    for (uint16_t row = 0; row < state->height; row++) {
        int16_t y = view->board_y_offset + (int16_t)row;
        if (y < 0 || y >= view->board_rows) {
            continue;
        }
        for (uint16_t col = 0; col < state->width; col++) {
            int8_t value = state->board[row * state->width + col];
            int8_t pidx = player_at(state, col, row);
            int16_t x = view->board_x_offset + col * CELL_WIDTH;

            if (pidx != NO_PLAYER) {
                wattron(view->board_win, COLOR_PAIR(pidx + COLOR_PAIR_OFFSET) | A_BOLD);
                mvwprintw(view->board_win, y, x, "P%d", pidx);
                wattroff(view->board_win, COLOR_PAIR(pidx + COLOR_PAIR_OFFSET) | A_BOLD);
            } else if (value > 0) {
                wattron(view->board_win, COLOR_PAIR(COLOR_BOARD));
                mvwprintw(view->board_win, y, x, "%2d", value);
                wattroff(view->board_win, COLOR_PAIR(COLOR_BOARD));
            } else {
                int8_t eater = (int8_t)(-value);
                wattron(view->board_win, COLOR_PAIR(eater + COLOR_PAIR_OFFSET) | A_DIM);
                mvwprintw(view->board_win, y, x, " *");
                wattroff(view->board_win, COLOR_PAIR(eater + COLOR_PAIR_OFFSET) | A_DIM);
            }

            if (col < state->width - 1) {
                mvwaddch(view->board_win, y, x + 2, ' ');
            }
        }
    }

    wrefresh(view->board_win);
}

static void draw_player_panel(WINDOW *win, int16_t x, int16_t w, player_t *player, int8_t idx) {
    int16_t color = idx + COLOR_PAIR_OFFSET;
    const char *name = display_name(player->name);
    const char *face = PLAYER_FACES[idx % MAX_PLAYERS];
    const char *status = player->state ? "ALIVE" : "DEAD";

    wattron(win, COLOR_PAIR(color));

    // Line 0: top border  +=== P0: naive [ALIVE] ===+
    mvwaddch(win, 0, x, '+');
    for (int16_t i = 1; i < w - 1; i++) {
        mvwaddch(win, 0, x + i, '=');
    }
    mvwaddch(win, 0, x + w - 1, '+');

    char header[LABEL_BUFFER_SIZE];
    snprintf(header, sizeof(header), " P%d: %s [%s] ", idx, name, status);
    int16_t hlen = (int16_t)strlen(header);
    int16_t hstart = x + (w - hlen) / 2;
    if (hstart < x + 1) {
        hstart = x + 1;
    }
    if (!player->state) {
        wattron(win, A_DIM);
    }
    mvwprintw(win, 0, hstart, "%s", header);
    if (!player->state) {
        wattroff(win, A_DIM);
    }

    // Line 1: face + score   | [^_^]  Score: 150     |
    mvwaddch(win, 1, x, '|');
    char line1[64];
    snprintf(line1, sizeof(line1), " %s  Score: %-5u", face, player->score);
    mvwprintw(win, 1, x + 1, "%-*s", w - 2, line1);
    mvwaddch(win, 1, x + w - 1, '|');

    // Line 2: pos + valid/invalid   | (5,3)  V:45  I:2    |
    mvwaddch(win, 2, x, '|');
    char line2[64];
    snprintf(line2, sizeof(line2), " (%u,%u)  V:%-4u I:%-4u", player->x, player->y, player->valid_moves,
             player->invalid_moves);
    mvwprintw(win, 2, x + 1, "%-*s", w - 2, line2);
    mvwaddch(win, 2, x + w - 1, '|');

    // Line 3: bottom border  +===...===+
    mvwaddch(win, 3, x, '+');
    for (int16_t i = 1; i < w - 1; i++) {
        mvwaddch(win, 3, x + i, '=');
    }
    mvwaddch(win, 3, x + w - 1, '+');

    wattroff(win, COLOR_PAIR(color));
}

void view_draw_panels(view_t *view, game_state_t *state) {
    werase(view->panel_win);

    if (state->players_count <= 0) {
        wrefresh(view->panel_win);
        return;
    }

    int16_t panel_w = view->term_cols / state->players_count;
    if (panel_w < 12) {
        panel_w = 12;
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
