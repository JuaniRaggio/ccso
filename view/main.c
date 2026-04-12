#include "error_management.h"
#include "view.h"
#include <errno.h>
#include <game.h>
#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

static char *get_endptr(char *str) {
    return str + strlen(str);
}

#define check_errno(endptr)                                                                                            \
    if (errno != 0 || *(endptr) != '\0') {                                                                             \
        manage_error(TRACE_NONE, HERE, invalid_argument_error);                                                        \
        return invalid_argument_error;                                                                                 \
    }

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    errno = 0;
    char *endptr = get_endptr(argv[1]);
    size_t width = strtoll(argv[1], &endptr, 10);
    check_errno(endptr);
    endptr = get_endptr(argv[2]);
    size_t height = strtoll(argv[2], &endptr, 10);
    check_errno(endptr);

    game_t game = new_game(view, .height = height, .width = width);
    view_t view;
    view_init(&view);

    while (!should_exit && game.state->running) {
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
