#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "test_suites.h"

#include <argv_parser.h>

/*
 * argv_parser exposes:
 *
 *   - parse_board_args(argv, &width, &height): parses argv[1] as width
 *     and argv[2] as height using strtoll with base 10. Returns true on
 *     success, false on any parsing error (overflow, trailing junk, etc.).
 *
 * The module is used by both the player and view binaries to receive
 * board dimensions from the master's fork+exec.
 */

/* ---------- parse_board_args ---------- */

/*
 * Happy path: two valid decimal integers produce the expected width and
 * height values.
 */
static void test_parse_board_args_happy_path(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"10", (char *)"20", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 10, (int)w);
    CuAssertIntEquals(tc, 20, (int)h);
}

/*
 * Zero is a valid uint16_t and must be accepted.
 */
static void test_parse_board_args_zero_values(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"0", (char *)"0", NULL};
    uint16_t w = 999, h = 999;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 0, (int)w);
    CuAssertIntEquals(tc, 0, (int)h);
}

/*
 * UINT16_MAX (65535) must be accepted.
 */
static void test_parse_board_args_max_uint16(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"65535", (char *)"65535", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 65535, (int)w);
    CuAssertIntEquals(tc, 65535, (int)h);
}

/*
 * Non-numeric width must cause failure. Height should not be parsed
 * because parse_board_args short-circuits with &&.
 */
static void test_parse_board_args_invalid_width(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"abc", (char *)"10", NULL};
    uint16_t w = 999, h = 999;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, !ok);
}

/*
 * Non-numeric height must cause failure.
 */
static void test_parse_board_args_invalid_height(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"10", (char *)"xyz", NULL};
    uint16_t w = 999, h = 999;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, !ok);
}

/*
 * Trailing junk after the number (e.g. "10abc") must be rejected.
 * strtoll sets endptr to the 'a', and *endptr != '\0' triggers failure.
 */
static void test_parse_board_args_trailing_junk(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"10abc", (char *)"20", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, !ok);
}

/*
 * A negative number like "-5" should be rejected by the ERANGE check
 * or the cast to uint16_t might produce a wrapped value. The strtoll
 * function itself accepts "-5" as -5 (no errno), and then the cast
 * to uint16_t wraps. But *endptr == '\0', so the current implementation
 * actually ACCEPTS negative numbers. This test documents the behavior.
 *
 * BUG: parse_uint16 accepts negative numbers because strtoll("-5", ...)
 * does not set errno and *endptr == '\0'. The cast (uint16_t)(-5) wraps
 * to 65531.
 */
static void test_parse_board_args_negative_is_accepted_bug(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"-5", (char *)"10", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    /* This documents the current behavior: negative values are silently accepted. */
    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, (int)(uint16_t)(-5), (int)w);
}

/*
 * Large overflow: a number exceeding LLONG_MAX should set ERANGE and
 * the function should return false.
 */
static void test_parse_board_args_overflow(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"99999999999999999999999", (char *)"10", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, !ok);
}

/*
 * A single character "1" must parse correctly.
 */
static void test_parse_board_args_single_digit(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"1", (char *)"2", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 1, (int)w);
    CuAssertIntEquals(tc, 2, (int)h);
}

/*
 * Leading whitespace is accepted by strtoll per POSIX, but the
 * parse_uint16 implementation initializes endptr to str + strlen(str)
 * before calling strtoll. This is suspicious because strtoll overwrites
 * endptr anyway. Let's verify the behavior with leading spaces.
 */
static void test_parse_board_args_leading_whitespace(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"  42", (char *)"  10", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 42, (int)w);
    CuAssertIntEquals(tc, 10, (int)h);
}

/*
 * "010" in base 10 is decimal 10, not octal 8. strtoll with base=10
 * does not interpret leading zeros as octal.
 */
static void test_parse_board_args_leading_zero_is_decimal(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"010", (char *)"020", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 10, (int)w);
    CuAssertIntEquals(tc, 20, (int)h);
}

/*
 * BUG: parse_uint16 does not range-check after strtoll. A value like
 * 70000 exceeds UINT16_MAX (65535) but strtoll does not set ERANGE
 * because it fits in long long. The cast (uint16_t)70000 wraps to
 * 70000 - 65536 = 4464. This test documents the truncation bug.
 */
static void test_parse_board_args_exceeds_uint16_wraps_bug(CuTest *tc) {
    char *argv[] = {(char *)"player", (char *)"70000", (char *)"10", NULL};
    uint16_t w = 0, h = 0;

    bool ok = parse_board_args(argv, &w, &h);

    /* The function returns true because strtoll does not set ERANGE. */
    CuAssertTrue(tc, ok);
    /* The value wraps: (uint16_t)70000 = 70000 - 65536 = 4464 */
    CuAssertIntEquals(tc, (int)(uint16_t)70000, (int)w);
}

CuSuite *argv_parser_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parse_board_args_happy_path);
    SUITE_ADD_TEST(suite, test_parse_board_args_zero_values);
    SUITE_ADD_TEST(suite, test_parse_board_args_max_uint16);
    SUITE_ADD_TEST(suite, test_parse_board_args_invalid_width);
    SUITE_ADD_TEST(suite, test_parse_board_args_invalid_height);
    SUITE_ADD_TEST(suite, test_parse_board_args_trailing_junk);
    SUITE_ADD_TEST(suite, test_parse_board_args_negative_is_accepted_bug);
    SUITE_ADD_TEST(suite, test_parse_board_args_overflow);
    SUITE_ADD_TEST(suite, test_parse_board_args_single_digit);
    SUITE_ADD_TEST(suite, test_parse_board_args_leading_whitespace);
    SUITE_ADD_TEST(suite, test_parse_board_args_leading_zero_is_decimal);
    SUITE_ADD_TEST(suite, test_parse_board_args_exceeds_uint16_wraps_bug);
    return suite;
}
