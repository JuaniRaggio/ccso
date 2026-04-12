#include "error_management.h"
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

#include <argv_parser.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uint16_t width, height;
    if (!parse_board_args(argv, &width, &height)) {
        return manage_error(TRACE_NONE, HERE, invalid_argument_error);
    }

    game_t game = new_game(view, .height = height, .width = width);

    if (getenv("TERM") == NULL) {
        setenv("TERM", "xterm-256color", 0);
    }

    view_t view;
    view_init(&view);

    while (1) {
        game_sync_view_wait_frame(game.sync);
        if (should_exit || !game.state->running) {
            break;
        }
        int32_t ch = getch();
        if (ch == KEY_RESIZE) {
            view_cleanup(&view);
            view_init(&view);
        }

        view_draw_all(&view, game.state);
        game_sync_view_frame_done(game.sync);
    }

    view_cleanup(&view);
    game_disconnect(&game);

    return 0;
}
