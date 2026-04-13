#include <player_movement.h>
#include <string.h>

static inline bool in_bounds(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    return x >= 0 && x < width && y >= 0 && y < height;
}

static inline int8_t board_at(int8_t board[], uint16_t width, int16_t x, int16_t y) {
    return board[y * width + x];
}

static inline bool is_free(int8_t cell) {
    return cell >= 1 && cell <= 9;
}

static inline void neighbor(int16_t x, int16_t y, int8_t dir, int16_t *nx, int16_t *ny) {
    apply_direction(x, y, dir, nx, ny);
}

static inline bool is_free_neighbor(int8_t board[], uint16_t width, uint16_t height, int16_t x, int16_t y, int8_t dir,
                                    int16_t *out_nx, int16_t *out_ny) {
    neighbor(x, y, dir, out_nx, out_ny);
    return in_bounds(*out_nx, *out_ny, width, height) && is_free(board_at(board, width, *out_nx, *out_ny));
}

#if defined(FLOOD) || defined(GREEDY_FLOOD)

static int32_t flood_count(int8_t board[], uint16_t width, uint16_t height, int16_t sx, int16_t sy) {
    int32_t total = width * height;
    uint8_t *visited = calloc(total, sizeof(uint8_t));
    if (!visited)
        return 0;

    int16_t *stack = malloc(total * 2 * sizeof(int16_t));
    if (!stack) {
        free(visited);
        return 0;
    }

    int32_t top = 0;
    stack[top++] = sx;
    stack[top++] = sy;
    visited[sy * width + sx] = 1;
    int32_t count = 0;

    while (top > 0) {
        int16_t cy = stack[--top];
        int16_t cx = stack[--top];
        int16_t nx, ny;
        for (int8_t d = 0; d < dir_count; d++) {
            if (!is_free_neighbor(board, width, height, cx, cy, d, &nx, &ny))
                continue;
            int32_t idx = ny * width + nx;
            if (visited[idx])
                continue;
            visited[idx] = 1;
            count++;
            stack[top++] = nx;
            stack[top++] = ny;
        }
    }

    free(stack);
    free(visited);
    return count;
}

#endif

#ifdef NAIVE

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    int8_t valid_dirs[dir_count];
    int8_t count = 0;
    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (is_free_neighbor(board, width, height, x, y, dir, &nx, &ny)) {
            valid_dirs[count++] = dir;
        }
    }
    return count > 0 ? valid_dirs[rand() % count] : NO_VALID_MOVE;
}

#elif defined(GREEDY)

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    int8_t best_dir = -1;
    int8_t best_val = 0;
    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (!is_free_neighbor(board, width, height, x, y, dir, &nx, &ny))
            continue;
        int8_t cell = board_at(board, width, nx, ny);
        if (cell > best_val) {
            best_val = cell;
            best_dir = dir;
        }
    }
    return best_dir >= 0 ? best_dir : NO_VALID_MOVE;
}

#elif defined(GREEDY_LOOKAHEAD)

#define LOOKAHEAD_DEPTH 3

static bool path_contains(int16_t path_x[], int16_t path_y[], int8_t path_len, int16_t x, int16_t y) {
    for (int8_t p = 0; p < path_len; p++) {
        if (path_x[p] == x && path_y[p] == y)
            return true;
    }
    return false;
}

static int32_t lookahead(int8_t board[], uint16_t width, uint16_t height, int16_t cx, int16_t cy, int8_t depth,
                         int16_t path_x[], int16_t path_y[], int8_t path_len) {
    if (depth == 0)
        return 0;
    int32_t best = 0;
    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (!is_free_neighbor(board, width, height, cx, cy, dir, &nx, &ny))
            continue;
        if (path_contains(path_x, path_y, path_len, nx, ny))
            continue;
        path_x[path_len] = nx;
        path_y[path_len] = ny;
        int32_t val = board_at(board, width, nx, ny) +
                      lookahead(board, width, height, nx, ny, depth - 1, path_x, path_y, path_len + 1);
        if (val > best)
            best = val;
    }
    return best;
}

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    int8_t best_dir = -1;
    int32_t best_val = 0;
    int16_t path_x[LOOKAHEAD_DEPTH + 1], path_y[LOOKAHEAD_DEPTH + 1];

    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (!is_free_neighbor(board, width, height, x, y, dir, &nx, &ny))
            continue;
        path_x[0] = nx;
        path_y[0] = ny;
        int32_t val = board_at(board, width, nx, ny) +
                      lookahead(board, width, height, nx, ny, LOOKAHEAD_DEPTH - 1, path_x, path_y, 1);
        if (val > best_val) {
            best_val = val;
            best_dir = dir;
        }
    }
    return best_dir >= 0 ? best_dir : NO_VALID_MOVE;
}

#elif defined(FLOOD)

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    int8_t best_dir = -1;
    int32_t best_space = -1;
    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (!is_free_neighbor(board, width, height, x, y, dir, &nx, &ny))
            continue;
        int32_t space = flood_count(board, width, height, nx, ny);
        if (space > best_space) {
            best_space = space;
            best_dir = dir;
        }
    }
    return best_dir >= 0 ? best_dir : NO_VALID_MOVE;
}

#elif defined(GREEDY_FLOOD)

#define SURVIVAL_NUM 3
#define SURVIVAL_DEN 10

static bool is_better_reward(int8_t r, int32_t f, int8_t best_r, int32_t best_f) {
    return r > best_r || (r == best_r && f > best_f);
}

static bool is_within_survival_threshold(int32_t flood, int32_t max_flood) {
    return flood >= (max_flood * SURVIVAL_NUM / SURVIVAL_DEN);
}

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    int32_t floods[dir_count] = {0}, max_flood = 0;
    int8_t rewards[dir_count] = {0};
    bool valid[dir_count] = {false};

    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (!is_free_neighbor(board, width, height, x, y, dir, &nx, &ny))
            continue;
        valid[dir] = true;
        rewards[dir] = board_at(board, width, nx, ny);
        floods[dir] = flood_count(board, width, height, nx, ny);
        if (floods[dir] > max_flood)
            max_flood = floods[dir];
    }

    int8_t best_dir = -1, best_reward = 0;
    int32_t best_flood = -1;

    for (int8_t d = 0; d < dir_count; d++) {
        if (!valid[d] || !is_within_survival_threshold(floods[d], max_flood))
            continue;
        if (is_better_reward(rewards[d], floods[d], best_reward, best_flood)) {
            best_reward = rewards[d];
            best_flood = floods[d];
            best_dir = d;
        }
    }

    if (best_dir == -1) {
        for (int8_t d = 0; d < dir_count; d++) {
            if (valid[d] && floods[d] > best_flood) {
                best_flood = floods[d];
                best_dir = d;
            }
        }
    }
    return best_dir >= 0 ? best_dir : NO_VALID_MOVE;
}

#elif defined(MIN_REWARD)

int8_t compute_next_move(int8_t board[], uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    int8_t best_dir = -1, min_val = 127;
    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (!is_free_neighbor(board, width, height, x, y, dir, &nx, &ny))
            continue;
        int8_t cell = board_at(board, width, nx, ny);
        if (cell < min_val) {
            min_val = cell;
            best_dir = dir;
        }
    }
    return best_dir >= 0 ? best_dir : NO_VALID_MOVE;
}

#endif

int8_t decide_move(game_state_t *state, uint16_t width, uint16_t height, uint16_t idx) {
    int16_t x = state->players[idx].x, y = state->players[idx].y;
    for (int8_t dir = 0; dir < dir_count; dir++) {
        int16_t nx, ny;
        if (is_free_neighbor(state->board, width, height, x, y, dir, &nx, &ny))
            return (uint8_t)dir;
    }
    return NO_VALID_MOVE;
}
