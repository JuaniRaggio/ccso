#include "player_protocol.h"

const move_delta_t move_delta[dir_count] = {
    [dir_up] = {0, -1},  [dir_up_right] = {1, -1},  [dir_right] = {1, 0}, [dir_down_right] = {1, 1},
    [dir_down] = {0, 1}, [dir_down_left] = {-1, 1}, [dir_left] = {-1, 0}, [dir_up_left] = {-1, -1},
};
