#include "view.h"
#include <game_state.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>

static inline int16_t elevation(int8_t cell_value) {
    if (cell_value < 1 || cell_value > 9) {
        return 0;
    }
    return (int16_t)((cell_value - 1) * HEX_MAX_ELEVATION / 8);
}

static int8_t player_at(game_state_t *state, uint16_t col, uint16_t row) {
    for (int8_t i = 0; i < state->players_count; i++) {
        if (state->players[i].x == col && state->players[i].y == row) {
            return i;
        }
    }
    return -1;
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
}
}
