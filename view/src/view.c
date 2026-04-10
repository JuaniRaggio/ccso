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
static void draw_player_panel(WINDOW *win, int16_t panel_row, player_t *player, int8_t idx) {
    int16_t y = panel_row * PANEL_HEIGHT;
    int16_t color = idx + 1;

    wattron(win, COLOR_PAIR(color));

    mvwprintw(win, y, 0, "+");
    for (int16_t i = 1; i < PANEL_WIDTH - 1; i++) {
        mvwaddch(win, y, i, '-');
    }
    mvwaddch(win, y, PANEL_WIDTH - 1, '+');

    char label[LABEL_BUFFER_SIZE];
    snprintf(label, sizeof(label), " P%d: %.10s ", idx, player->name);
    int16_t label_start = (PANEL_WIDTH - (int16_t)strlen(label)) / 2;
    mvwprintw(win, y, label_start, "%s", label);

    mvwprintw(win, y + 1, 0, "| %s  Score: %-6u      |",
              PLAYER_FACES[idx % MAX_PLAYERS], player->score);

    mvwprintw(win, y + 2, 0, "| Pos: (%-3u, %-3u)        |", player->x, player->y);

    mvwprintw(win, y + 3, 0, "| Moves: %-5uv %-5ui   |",
              player->valid_moves, player->invalid_moves);

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
