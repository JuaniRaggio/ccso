#include <argv_parser.h>
#include <error_management.h>
#include <game.h>
#include <player_loop.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    uint16_t width, height;
    if (!parse_board_args(argv, &width, &height)) {
        return manage_error(TRACE_NONE, HERE, invalid_argument_error);
    }

    game_t game = new_game(player, .height = height, .width = width);
    player_run(&game);
    game_disconnect(&game);

    return 0;
}
