#include "player_protocol.h"

const move_delta_t move_delta[dir_count] = {
    [dir_up] = {0, -1},  [dir_up_right] = {1, -1},  [dir_right] = {1, 0}, [dir_down_right] = {1, 1},
    [dir_down] = {0, 1}, [dir_down_left] = {-1, 1}, [dir_left] = {-1, 0}, [dir_up_left] = {-1, -1},
};

ssize_t send_direction(int32_t fd, direction_wire_t dir) {
    uint8_t byte = (uint8_t)dir;
    return write(fd, &byte, sizeof(byte));
}

ssize_t recv_direction(int32_t fd, direction_wire_t *out_dir) {
    uint8_t byte;
    ssize_t n = read(fd, &byte, sizeof(byte));
    if (n == 1) {
        *out_dir = (direction_wire_t)byte;
    }
    return n;
}
