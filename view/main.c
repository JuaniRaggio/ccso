#include "error_management.h"
#include "view.h"
#include <game.h>
#include <game_state.h>
#include <game_sync.h>
#include <locale.h>
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

static void setup_signals() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

static void initialize_game_view(view_t *view, uint16_t width, uint16_t height) {
    if (getenv("TERM") == NULL) {
        setenv("TERM", "xterm-256color", 0);
    }
    view_init(view, width, height);
}

static void handle_resize(view_t *view, uint16_t width, uint16_t height) {
    view_cleanup(view);
    view_init(view, width, height);
}

static void run_game_loop(view_t *view, game_t *game, uint16_t width, uint16_t height) {
    while (1) {
        game_sync_view_wait_frame(game->sync);
        if (should_exit || !game->state->running) {
            game_sync_view_frame_done(game->sync);
            break;
        }

        if (getch() == KEY_RESIZE) {
            handle_resize(view, width, height);
        }

        view_draw_all(view, game->state);
        game_sync_view_frame_done(game->sync);
    }
}

static void show_final_results(view_t *view, game_state_t *state) {
    if (should_exit)
        return;
    view_draw_all(view, state);
    view_draw_endscreen(view, state);
    nodelay(stdscr, FALSE);
    getch();
}

int main(int argc, char *argv[]) {
    if (!setlocale(LC_ALL, "C.utf8")) {
        setlocale(LC_ALL, "");
    }
    setup_signals();

    uint16_t width, height;
    if (!parse_board_args(argv, &width, &height)) {
        return manage_error(TRACE_NONE, HERE, invalid_argument_error);
    }

    game_t game = new_game(view, .height = height, .width = width);
    view_t game_view;
    initialize_game_view(&game_view, width, height);

    run_game_loop(&game_view, &game, width, height);
    show_final_results(&game_view, game.state);

    view_cleanup(&game_view);
    game_disconnect(&game);

    return 0;
}
