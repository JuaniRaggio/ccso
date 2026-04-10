#include "view.h"
#include <game.h>
#include <game_state.h>
#include <game_sync.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    game_t game = new_game(view);


    view_t view;
    view_init(&view);

    while (!should_exit && game.state->running) {
        if (should_exit || !game.state->running) {
            break;
        }
    }

    view_cleanup(&view);
    game_disconnect(&game);

    return 0;
}
