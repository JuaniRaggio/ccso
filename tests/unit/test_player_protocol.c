#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "CuTest.h"
#include "test_suites.h"

#include <player_protocol.h>

/*
 * player_protocol defines:
 *
 *   - direction_t: an enum of 8 compass directions plus dir_count (= 8).
 *   - move_delta: lookup table mapping direction_t to a move_delta_t struct
 *     containing dx (column delta) and dy (row delta). By convention,
 *     "up" means negative Y (toward row 0).
 *   - is_valid_direction: returns true iff d is in [0, dir_count).
 *   - apply_direction: given (x, y) and a direction, returns (out_x, out_y).
 *   - NO_VALID_MOVE: sentinel value (-1) for "no move available".
 *
 * The module is self-contained with no external dependencies beyond libc,
 * so every function and array is exercisable directly.
 */

/* ---------- enum layout ---------- */

/*
 * The direction_t enum must be sequential starting from 0, and dir_count
 * must equal 8 (the number of compass directions). This is a structural
 * invariant that the lookup tables and all strategy code depend on.
 */
static void test_direction_enum_layout(CuTest *tc) {
    CuAssertIntEquals(tc, 0, (int)dir_up);
    CuAssertIntEquals(tc, 1, (int)dir_up_right);
    CuAssertIntEquals(tc, 2, (int)dir_right);
    CuAssertIntEquals(tc, 3, (int)dir_down_right);
    CuAssertIntEquals(tc, 4, (int)dir_down);
    CuAssertIntEquals(tc, 5, (int)dir_down_left);
    CuAssertIntEquals(tc, 6, (int)dir_left);
    CuAssertIntEquals(tc, 7, (int)dir_up_left);
    CuAssertIntEquals(tc, 8, (int)dir_count);
}

/* ---------- move_delta table ---------- */

/*
 * Verify the dx (horizontal delta) for each direction.
 * Right is +1, left is -1, vertical-only is 0.
 */
static void test_move_dx_values(CuTest *tc) {
    CuAssertIntEquals(tc, 0, (int)move_delta[dir_up].dx);
    CuAssertIntEquals(tc, 1, (int)move_delta[dir_up_right].dx);
    CuAssertIntEquals(tc, 1, (int)move_delta[dir_right].dx);
    CuAssertIntEquals(tc, 1, (int)move_delta[dir_down_right].dx);
    CuAssertIntEquals(tc, 0, (int)move_delta[dir_down].dx);
    CuAssertIntEquals(tc, -1, (int)move_delta[dir_down_left].dx);
    CuAssertIntEquals(tc, -1, (int)move_delta[dir_left].dx);
    CuAssertIntEquals(tc, -1, (int)move_delta[dir_up_left].dx);
}

/*
 * Verify the dy (vertical delta) for each direction.
 * Up is -1, down is +1, horizontal-only is 0.
 */
static void test_move_dy_values(CuTest *tc) {
    CuAssertIntEquals(tc, -1, (int)move_delta[dir_up].dy);
    CuAssertIntEquals(tc, -1, (int)move_delta[dir_up_right].dy);
    CuAssertIntEquals(tc, 0, (int)move_delta[dir_right].dy);
    CuAssertIntEquals(tc, 1, (int)move_delta[dir_down_right].dy);
    CuAssertIntEquals(tc, 1, (int)move_delta[dir_down].dy);
    CuAssertIntEquals(tc, 1, (int)move_delta[dir_down_left].dy);
    CuAssertIntEquals(tc, 0, (int)move_delta[dir_left].dy);
    CuAssertIntEquals(tc, -1, (int)move_delta[dir_up_left].dy);
}

/*
 * Every direction must have |dx| <= 1 and |dy| <= 1 (king moves on a
 * chess board). This is a structural invariant of the protocol.
 */
static void test_move_deltas_bounded(CuTest *tc) {
    for (int32_t d = 0; d < (int32_t)dir_count; d++) {
        CuAssertTrue(tc, move_delta[d].dx >= -1 && move_delta[d].dx <= 1);
        CuAssertTrue(tc, move_delta[d].dy >= -1 && move_delta[d].dy <= 1);
    }
}

/*
 * No two directions should produce the same (dx, dy) pair. This
 * guarantees the mapping is injective: every compass direction moves
 * to a unique neighbor cell.
 */
static void test_move_deltas_all_distinct(CuTest *tc) {
    for (int32_t a = 0; a < (int32_t)dir_count; a++) {
        for (int32_t b = a + 1; b < (int32_t)dir_count; b++) {
            bool same = move_delta[a].dx == move_delta[b].dx && move_delta[a].dy == move_delta[b].dy;
            CuAssertTrue(tc, !same);
        }
    }
}

/*
 * Opposite directions must produce negated deltas:
 *   up       <-> down
 *   up_right <-> down_left
 *   right    <-> left
 *   down_right <-> up_left
 */
static void test_opposite_directions_negate(CuTest *tc) {
    CuAssertIntEquals(tc, -move_delta[dir_up].dx, move_delta[dir_down].dx);
    CuAssertIntEquals(tc, -move_delta[dir_up].dy, move_delta[dir_down].dy);

    CuAssertIntEquals(tc, -move_delta[dir_up_right].dx, move_delta[dir_down_left].dx);
    CuAssertIntEquals(tc, -move_delta[dir_up_right].dy, move_delta[dir_down_left].dy);

    CuAssertIntEquals(tc, -move_delta[dir_right].dx, move_delta[dir_left].dx);
    CuAssertIntEquals(tc, -move_delta[dir_right].dy, move_delta[dir_left].dy);

    CuAssertIntEquals(tc, -move_delta[dir_down_right].dx, move_delta[dir_up_left].dx);
    CuAssertIntEquals(tc, -move_delta[dir_down_right].dy, move_delta[dir_up_left].dy);
}

/* ---------- apply_direction ---------- */

/*
 * apply_direction must correctly offset coordinates by the delta of the
 * given direction.
 */
static void test_apply_direction_basic(CuTest *tc) {
    int16_t out_x, out_y;

    apply_direction(5, 5, dir_up, &out_x, &out_y);
    CuAssertIntEquals(tc, 5, (int)out_x);
    CuAssertIntEquals(tc, 4, (int)out_y);

    apply_direction(5, 5, dir_down_right, &out_x, &out_y);
    CuAssertIntEquals(tc, 6, (int)out_x);
    CuAssertIntEquals(tc, 6, (int)out_y);

    apply_direction(0, 0, dir_left, &out_x, &out_y);
    CuAssertIntEquals(tc, -1, (int)out_x);
    CuAssertIntEquals(tc, 0, (int)out_y);
}

/* ---------- is_valid_direction ---------- */

/*
 * All 8 compass directions must be valid.
 */
static void test_is_valid_direction_all_valid(CuTest *tc) {
    for (int32_t d = 0; d < (int32_t)dir_count; d++) {
        CuAssertTrue(tc, is_valid_direction((direction_wire_t)d));
    }
}

/*
 * dir_count itself (8) is out of range and must be invalid.
 */
static void test_is_valid_direction_dir_count_is_invalid(CuTest *tc) {
    CuAssertTrue(tc, !is_valid_direction((direction_wire_t)dir_count));
}

/*
 * -1 (NO_VALID_MOVE) must be invalid.
 */
static void test_is_valid_direction_negative_is_invalid(CuTest *tc) {
    CuAssertTrue(tc, !is_valid_direction(NO_VALID_MOVE));
    CuAssertTrue(tc, !is_valid_direction(-1));
    CuAssertTrue(tc, !is_valid_direction(-128));
}

/*
 * Large positive values must be invalid.
 */
static void test_is_valid_direction_large_positive_is_invalid(CuTest *tc) {
    CuAssertTrue(tc, !is_valid_direction(8));
    CuAssertTrue(tc, !is_valid_direction(100));
    CuAssertTrue(tc, !is_valid_direction(127));
}

/*
 * Boundary: 0 is the first valid direction (dir_up), and dir_count - 1
 * (= 7, dir_up_left) is the last valid direction.
 */
static void test_is_valid_direction_boundaries(CuTest *tc) {
    CuAssertTrue(tc, is_valid_direction(0));
    CuAssertTrue(tc, is_valid_direction((direction_wire_t)(dir_count - 1)));
    CuAssertTrue(tc, !is_valid_direction(-1));
    CuAssertTrue(tc, !is_valid_direction((direction_wire_t)dir_count));
}

/* ---------- NO_VALID_MOVE ---------- */

/*
 * NO_VALID_MOVE must equal -1 and must fail is_valid_direction.
 */
static void test_no_valid_move_sentinel(CuTest *tc) {
    CuAssertIntEquals(tc, -1, (int)NO_VALID_MOVE);
    CuAssertTrue(tc, !is_valid_direction(NO_VALID_MOVE));
}

/* ---------- send_direction / recv_direction ---------- */

/*
 * Helper: create a pipe and return the read/write fds.
 * Returns 0 on success, -1 on failure.
 */
static int32_t make_pipe(int32_t *read_fd, int32_t *write_fd) {
    int32_t fds[2];
    if (pipe(fds) == -1) {
        return -1;
    }
    *read_fd = fds[0];
    *write_fd = fds[1];
    return 0;
}

/*
 * send_direction followed by recv_direction must roundtrip a single
 * direction value correctly.
 */
static void test_send_recv_direction_roundtrip(CuTest *tc) {
    int32_t rfd, wfd;
    CuAssertIntEquals(tc, 0, make_pipe(&rfd, &wfd));

    direction_wire_t sent = dir_right;
    ssize_t written = send_direction(wfd, sent);
    CuAssertIntEquals(tc, 1, (int)written);

    direction_wire_t received = -99;
    ssize_t read_n = recv_direction(rfd, &received);
    CuAssertIntEquals(tc, 1, (int)read_n);
    CuAssertIntEquals(tc, (int)sent, (int)received);

    close(rfd);
    close(wfd);
}

/*
 * All 8 valid directions must roundtrip correctly through a pipe.
 */
static void test_send_recv_all_directions(CuTest *tc) {
    int32_t rfd, wfd;
    CuAssertIntEquals(tc, 0, make_pipe(&rfd, &wfd));

    for (int8_t d = 0; d < (int8_t)dir_count; d++) {
        ssize_t written = send_direction(wfd, (direction_wire_t)d);
        CuAssertIntEquals(tc, 1, (int)written);
    }

    for (int8_t d = 0; d < (int8_t)dir_count; d++) {
        direction_wire_t received = -99;
        ssize_t read_n = recv_direction(rfd, &received);
        CuAssertIntEquals(tc, 1, (int)read_n);
        CuAssertIntEquals(tc, (int)d, (int)received);
    }

    close(rfd);
    close(wfd);
}

/*
 * NO_VALID_MOVE (-1) must roundtrip correctly. send_direction casts
 * to uint8_t, so -1 becomes 0xFF. recv_direction casts back to
 * direction_wire_t (int8_t), which is -1 on two's complement.
 */
static void test_send_recv_no_valid_move(CuTest *tc) {
    int32_t rfd, wfd;
    CuAssertIntEquals(tc, 0, make_pipe(&rfd, &wfd));

    ssize_t written = send_direction(wfd, NO_VALID_MOVE);
    CuAssertIntEquals(tc, 1, (int)written);

    direction_wire_t received = 0;
    ssize_t read_n = recv_direction(rfd, &received);
    CuAssertIntEquals(tc, 1, (int)read_n);
    CuAssertIntEquals(tc, (int)NO_VALID_MOVE, (int)received);

    close(rfd);
    close(wfd);
}

/*
 * recv_direction on a closed pipe (EOF) must return 0 and not modify
 * the output parameter.
 */
static void test_recv_direction_eof(CuTest *tc) {
    int32_t rfd, wfd;
    CuAssertIntEquals(tc, 0, make_pipe(&rfd, &wfd));

    close(wfd); /* close write end -> EOF on read end */

    direction_wire_t received = 42;
    ssize_t read_n = recv_direction(rfd, &received);
    CuAssertIntEquals(tc, 0, (int)read_n);
    /* received should NOT have been modified (n != 1) */
    CuAssertIntEquals(tc, 42, (int)received);

    close(rfd);
}

/*
 * send_direction on an invalid fd should return -1 (error).
 */
static void test_send_direction_bad_fd(CuTest *tc) {
    ssize_t result = send_direction(-1, dir_up);
    CuAssertTrue(tc, result == -1);
}

CuSuite *player_protocol_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_direction_enum_layout);
    SUITE_ADD_TEST(suite, test_move_dx_values);
    SUITE_ADD_TEST(suite, test_move_dy_values);
    SUITE_ADD_TEST(suite, test_move_deltas_bounded);
    SUITE_ADD_TEST(suite, test_move_deltas_all_distinct);
    SUITE_ADD_TEST(suite, test_opposite_directions_negate);
    SUITE_ADD_TEST(suite, test_apply_direction_basic);
    SUITE_ADD_TEST(suite, test_is_valid_direction_all_valid);
    SUITE_ADD_TEST(suite, test_is_valid_direction_dir_count_is_invalid);
    SUITE_ADD_TEST(suite, test_is_valid_direction_negative_is_invalid);
    SUITE_ADD_TEST(suite, test_is_valid_direction_large_positive_is_invalid);
    SUITE_ADD_TEST(suite, test_is_valid_direction_boundaries);
    SUITE_ADD_TEST(suite, test_no_valid_move_sentinel);
    /* send_direction / recv_direction */
    SUITE_ADD_TEST(suite, test_send_recv_direction_roundtrip);
    SUITE_ADD_TEST(suite, test_send_recv_all_directions);
    SUITE_ADD_TEST(suite, test_send_recv_no_valid_move);
    SUITE_ADD_TEST(suite, test_recv_direction_eof);
    SUITE_ADD_TEST(suite, test_send_direction_bad_fd);
    return suite;
}
