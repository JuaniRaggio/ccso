#include "view.h"
#include <game.h>
#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    (void)sig;
    should_exit = 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    game_t game = new_game(view);

    view_t view;
    view_init(&view);

    while (!should_exit && game.state->running) {
        sem_wait(&game.sync->pending_changes_to_show);
        if (should_exit || !game.state->running) {
            break;
        }
        int32_t ch = getch();
        if (ch == KEY_RESIZE) {
            view_cleanup(&view);
            view_init(&view);
        }

        view_draw_all(&view, game.state);
        sem_post(&game.sync->printing);
    }

    view_cleanup(&view);
    game_disconnect(&game);

    return 0;
}
