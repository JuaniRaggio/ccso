#include "error_management.h"
#include "view.h"
#include <argv_parser.h>
#include <game.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    uint16_t width, height;
    if (!parse_board_args(argv, &width, &height)) {
        return manage_error(TRACE_NONE, HERE, invalid_argument_error);
    }

    game_t game = new_game(view, .height = height, .width = width);
    view_t game_view;

    view_init(&game_view, width, height);
    view_run(&game_view, &game, width, height);
    view_show_results(&game_view, game.state);
    view_cleanup(&game_view);

    game_disconnect(&game);
    return 0;
}
