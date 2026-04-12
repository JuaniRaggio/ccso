#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "test_suites.h"

#include <game_state.h>
#include <player_movement.h>
#include <player_protocol.h>

/*
 * player_movement exposes:
 *
 *   - compute_next_move(board, width, height, x, y): returns a direction
 *     (0..7) or NO_VALID_MOVE (-1). The behavior depends on the strategy
 *     selected at compile time via #ifdef (NAIVE, GREEDY, etc.).
 *
 *   - decidir_movimiento(state, width, height, idx): reads player position
 *     from state->players[idx] and finds the first valid neighbor direction
 *     in order 0..7. Returns that direction or NO_VALID_MOVE.
 *
 * This test file is compiled with -DGREEDY, so compute_next_move uses the
 * greedy strategy: pick the adjacent cell with the highest reward value.
 *
 * Helper functions in_bounds, board_at, is_free, is_free_neighbor are
 * all static inline in player_movement.c and are exercised indirectly
 * through compute_next_move and decidir_movimiento.
 */

/* Helper: create a flat board of width*height filled with a given value. */
static int8_t *make_board(uint16_t width, uint16_t height, int8_t fill) {
    int32_t total = (int32_t)width * height;
    int8_t *board = malloc(total);
    if (board != NULL) {
        memset(board, fill, total);
    }
    return board;
}

/* Helper: allocate a game_state_t with flexible array member for the board. */
static game_state_t *make_state(uint16_t width, uint16_t height, int8_t fill) {
    size_t size = sizeof(game_state_t) + (size_t)width * height;
    game_state_t *state = calloc(1, size);
    if (state != NULL) {
        state->width = width;
        state->height = height;
        memset(state->board, fill, (size_t)width * height);
    }
    return state;
}

/* ---------- compute_next_move (GREEDY) ---------- */

/*
 * On a uniform board (all cells equal), greedy should still pick a
 * direction (the first one it finds with the max value). It should
 * not return NO_VALID_MOVE because all neighbors are free.
 */
static void test_greedy_uniform_board_picks_something(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    int8_t *board = make_board(w, h, 5);
    CuAssertPtrNotNull(tc, board);

    int8_t result = compute_next_move(board, w, h, 2, 2);
    CuAssertTrue(tc, result >= 0 && result < (int8_t)dir_count);

    free(board);
}

/*
 * If exactly one neighbor has a strictly higher value, greedy must
 * pick that direction.
 */
static void test_greedy_picks_highest_neighbor(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    int8_t *board = make_board(w, h, 1);
    CuAssertPtrNotNull(tc, board);

    /* Place a 9 at (3, 2) -- that is to the right of (2, 2). */
    board[2 * w + 3] = 9;

    int8_t result = compute_next_move(board, w, h, 2, 2);
    CuAssertIntEquals(tc, (int)dir_right, (int)result);

    free(board);
}

/*
 * Greedy looking at cell above: place highest value at (2, 1),
 * which is dir_up from (2, 2).
 */
static void test_greedy_picks_up(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    int8_t *board = make_board(w, h, 1);
    CuAssertPtrNotNull(tc, board);

    /* (x=2, y=1) is dir_up from (x=2, y=2). board index: y*w+x = 1*5+2=7 */
    board[1 * w + 2] = 9;

    int8_t result = compute_next_move(board, w, h, 2, 2);
    CuAssertIntEquals(tc, (int)dir_up, (int)result);

    free(board);
}

/*
 * When the player is completely surrounded by claimed cells (board < 0),
 * greedy must return NO_VALID_MOVE because no neighbor has is_free(cell).
 * is_free checks cell >= 1 && cell <= 9, so negatives and zeros fail.
 */
static void test_greedy_completely_surrounded(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    int8_t *board = make_board(w, h, 1);
    CuAssertPtrNotNull(tc, board);

    /* Claim all 8 neighbors of (2, 2). */
    board[1 * w + 1] = -1; /* up-left */
    board[1 * w + 2] = -1; /* up */
    board[1 * w + 3] = -1; /* up-right */
    board[2 * w + 1] = -1; /* left */
    board[2 * w + 3] = -1; /* right */
    board[3 * w + 1] = -1; /* down-left */
    board[3 * w + 2] = -1; /* down */
    board[3 * w + 3] = -1; /* down-right */

    int8_t result = compute_next_move(board, w, h, 2, 2);
    CuAssertIntEquals(tc, (int)NO_VALID_MOVE, (int)result);

    free(board);
}

/*
 * Player at corner (0, 0): only 3 neighbors (right, down-right, down)
 * are in bounds. The one with the highest value must be chosen.
 */
static void test_greedy_corner_position(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    int8_t *board = make_board(w, h, 1);
    CuAssertPtrNotNull(tc, board);

    /* Only right (1,0), down-right (1,1), down (0,1) are in bounds from (0,0). */
    /* Place highest at (1, 1) = down-right. board index: 1*5+1=6 */
    board[1 * w + 1] = 9;

    int8_t result = compute_next_move(board, w, h, 0, 0);
    CuAssertIntEquals(tc, (int)dir_down_right, (int)result);

    free(board);
}

/*
 * 1x1 board: there are no neighbors at all. Must return NO_VALID_MOVE.
 */
static void test_greedy_1x1_board(CuTest *tc) {
    const uint16_t w = 1, h = 1;
    int8_t *board = make_board(w, h, 5);
    CuAssertPtrNotNull(tc, board);

    int8_t result = compute_next_move(board, w, h, 0, 0);
    CuAssertIntEquals(tc, (int)NO_VALID_MOVE, (int)result);

    free(board);
}

/*
 * Greedy should prefer higher cell values. With two free neighbors,
 * the one with value 8 must be chosen over value 2.
 */
static void test_greedy_prefers_higher_value(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    int8_t *board = make_board(w, h, -1); /* all claimed except what we set */
    CuAssertPtrNotNull(tc, board);

    /* Only two free cells: right (3,2)=2, down (2,3)=8 */
    board[2 * w + 3] = 2; /* right from (2,2) */
    board[3 * w + 2] = 8; /* down from (2,2) */

    int8_t result = compute_next_move(board, w, h, 2, 2);
    CuAssertIntEquals(tc, (int)dir_down, (int)result);

    free(board);
}

/* ---------- decidir_movimiento ---------- */

/*
 * decidir_movimiento uses the player's position from state->players[idx]
 * and finds the first free neighbor direction in order 0..7.
 */
static void test_decidir_movimiento_first_free_direction(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    game_state_t *state = make_state(w, h, 1);
    CuAssertPtrNotNull(tc, state);

    state->players[0].x = 2;
    state->players[0].y = 2;

    int8_t result = decidir_movimiento(state, w, h, 0);
    /* All neighbors free, so the first direction (dir_up=0) should be chosen
       because the loop goes 0..7 and returns the first free one. */
    CuAssertIntEquals(tc, (int)dir_up, (int)result);

    free(state);
}

/*
 * If only one direction is free (e.g. down), decidir_movimiento must
 * pick that direction.
 */
static void test_decidir_movimiento_only_one_free(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    game_state_t *state = make_state(w, h, -1); /* all claimed */
    CuAssertPtrNotNull(tc, state);

    state->players[0].x = 2;
    state->players[0].y = 2;

    /* Make only the down cell (2, 3) free. board index: 3*5+2 = 17. */
    state->board[3 * w + 2] = 5;

    int8_t result = decidir_movimiento(state, w, h, 0);
    CuAssertIntEquals(tc, (int)dir_down, (int)result);

    free(state);
}

/*
 * Completely surrounded: decidir_movimiento must return NO_VALID_MOVE.
 */
static void test_decidir_movimiento_surrounded(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    game_state_t *state = make_state(w, h, -1); /* all claimed */
    CuAssertPtrNotNull(tc, state);

    state->players[0].x = 2;
    state->players[0].y = 2;

    int8_t result = decidir_movimiento(state, w, h, 0);
    CuAssertIntEquals(tc, (int)NO_VALID_MOVE, (int)result);

    free(state);
}

/*
 * Corner position (0, 0): only right, down-right, and down are in bounds.
 * If all three are free, the first in enum order is dir_right (index 2),
 * because dir_up (0) and dir_up_right (1) are out of bounds.
 */
static void test_decidir_movimiento_corner(CuTest *tc) {
    const uint16_t w = 5, h = 5;
    game_state_t *state = make_state(w, h, 5);
    CuAssertPtrNotNull(tc, state);

    state->players[0].x = 0;
    state->players[0].y = 0;

    int8_t result = decidir_movimiento(state, w, h, 0);
    /* dir_up (dy=-1 -> y=-1 OOB), dir_up_right (OOB), dir_right (1,0 valid).
       First valid is dir_right (index 2). */
    CuAssertIntEquals(tc, (int)dir_right, (int)result);

    free(state);
}

CuSuite *player_movement_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    /* compute_next_move (GREEDY) */
    SUITE_ADD_TEST(suite, test_greedy_uniform_board_picks_something);
    SUITE_ADD_TEST(suite, test_greedy_picks_highest_neighbor);
    SUITE_ADD_TEST(suite, test_greedy_picks_up);
    SUITE_ADD_TEST(suite, test_greedy_completely_surrounded);
    SUITE_ADD_TEST(suite, test_greedy_corner_position);
    SUITE_ADD_TEST(suite, test_greedy_1x1_board);
    SUITE_ADD_TEST(suite, test_greedy_prefers_higher_value);
    /* decidir_movimiento */
    SUITE_ADD_TEST(suite, test_decidir_movimiento_first_free_direction);
    SUITE_ADD_TEST(suite, test_decidir_movimiento_only_one_free);
    SUITE_ADD_TEST(suite, test_decidir_movimiento_surrounded);
    SUITE_ADD_TEST(suite, test_decidir_movimiento_corner);
    return suite;
}
