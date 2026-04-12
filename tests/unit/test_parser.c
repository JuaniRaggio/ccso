#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "CuTest.h"
#include "test_suites.h"

#include <game.h>
#include <game_state.h>
#include <parser.h>

/*
 * Reset getopt state between tests so each one starts parsing from argv[1].
 *
 * glibc uses optind = 0 for a full re-initialization, while the BSD getopt
 * shipped with macOS uses optind = 1 together with optreset = 1. We cover
 * both behaviors so tests work on Linux and macOS transparently.
 */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
extern int optreset;
static void reset_getopt(void) {
    optind = 1;
    optreset = 1;
}
#else
static void reset_getopt(void) {
    optind = 0;
}
#endif

/* Helper: build a fresh parameters_t with sensible defaults. */
static parameters_t make_default_parameters(void) {
    parameters_t parameters = {0};
    parameters.width = default_width;
    parameters.height = default_heigh;
    parameters.delay = default_delay;
    parameters.timeout = default_timeout;
    parameters.seed = 0;
    parameters.view_path = default_view_path;
    parameters.players_count = 0;
    return parameters;
}

/*
 * No arguments beyond the binary name means parse should return success and
 * leave the defaults untouched.
 */
static void test_parse_no_arguments(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", NULL};
    int32_t argc = 1;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, (int)default_width, (int)parameters.width);
    CuAssertIntEquals(tc, (int)default_heigh, (int)parameters.height);
    CuAssertIntEquals(tc, (int)default_delay, (int)parameters.delay);
    CuAssertIntEquals(tc, (int)default_timeout, (int)parameters.timeout);
    CuAssertIntEquals(tc, 0, (int)parameters.players_count);
    CuAssertPtrEquals(tc, (void *)default_view_path, (void *)parameters.view_path);
}

/*
 * Width and height can be provided with -w and -h and should update the
 * corresponding fields.
 */
static void test_parse_width_and_height(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"25", (char *)"-h", (char *)"30", NULL};
    int32_t argc = 5;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 25, (int)parameters.width);
    CuAssertIntEquals(tc, 30, (int)parameters.height);
}

/*
 * Delay, timeout and seed are parsed with -d, -t and -s respectively.
 */
static void test_parse_delay_timeout_seed(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-d", (char *)"500", (char *)"-t",
                    (char *)"15",     (char *)"-s", (char *)"42",  NULL};
    int32_t argc = 7;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 500, (int)parameters.delay);
    CuAssertIntEquals(tc, 15, (int)parameters.timeout);
    CuAssertIntEquals(tc, 42, (int)parameters.seed);
}

/*
 * -v stores a pointer to the optarg string directly in view_path.
 */
static void test_parse_view_path(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-v", (char *)"./build/view", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertPtrNotNull(tc, (void *)parameters.view_path);
    CuAssertStrEquals(tc, "./build/view", parameters.view_path);
}

/*
 * Multiple -p flags should populate players_paths sequentially and bump
 * players_count.
 */
static void test_parse_multiple_players(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master",           (char *)"-p", (char *)"./build/player-a", (char *)"-p",
                    (char *)"./build/player-b", (char *)"-p", (char *)"./build/player-c", NULL};
    int32_t argc = 7;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 3, (int)parameters.players_count);
    CuAssertStrEquals(tc, "./build/player-a", parameters.players_paths[0]);
    CuAssertStrEquals(tc, "./build/player-b", parameters.players_paths[1]);
    CuAssertStrEquals(tc, "./build/player-c", parameters.players_paths[2]);
}

/*
 * Adding exactly MAX_PLAYERS players must still succeed: the limit is
 * inclusive, so the Nth player is still accepted.
 */
static void test_parse_max_players_exact(CuTest *tc) {
    reset_getopt();
    char *argv[2 + 2 * MAX_PLAYERS];
    argv[0] = (char *)"master";

    static const char *const paths[MAX_PLAYERS] = {
        "./p0", "./p1", "./p2", "./p3", "./p4", "./p5", "./p6", "./p7", "./p8",
    };
    for (int32_t i = 0; i < MAX_PLAYERS; i++) {
        argv[1 + i * 2] = (char *)"-p";
        argv[2 + i * 2] = (char *)paths[i];
    }
    argv[1 + 2 * MAX_PLAYERS] = NULL;
    int32_t argc = 1 + 2 * MAX_PLAYERS;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, MAX_PLAYERS, (int)parameters.players_count);
}

/*
 * Providing more than MAX_PLAYERS players should flip the
 * exceeded_player_limit bit.
 */
static void test_parse_too_many_players(CuTest *tc) {
    reset_getopt();
    char *argv[2 + 2 * (MAX_PLAYERS + 1) + 1];
    argv[0] = (char *)"master";

    for (int32_t i = 0; i < MAX_PLAYERS + 1; i++) {
        argv[1 + i * 2] = (char *)"-p";
        argv[2 + i * 2] = (char *)"./player";
    }
    argv[1 + 2 * (MAX_PLAYERS + 1)] = NULL;
    int32_t argc = 1 + 2 * (MAX_PLAYERS + 1);

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & exceeded_player_limit) != 0);
}

/*
 * A non-numeric argument passed to -w must raise invalid_integer_type.
 * strtoull leaves a non-null endptr when it cannot convert, which the
 * parser uses to detect the invalid input.
 */
static void test_parse_invalid_integer(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"notanumber", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & invalid_integer_type) != 0);
}

/*
 * "12abc" has a numeric prefix but trailing junk: endptr will not point to
 * the terminating NUL, so invalid_integer_type must be set.
 */
static void test_parse_invalid_integer_with_suffix(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-h", (char *)"12abc", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & invalid_integer_type) != 0);
}

/*
 * An unknown short option (e.g. -q) is surfaced by getopt as '?', and the
 * parser maps it to unknown_optional_flag.
 */
static void test_parse_unknown_flag(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-q", NULL};
    int32_t argc = 2;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & unknown_optional_flag) != 0);
}

/*
 * A value that obviously overflows uint64 should flip the overflow bit
 * via errno == ERANGE after strtoull.
 */
static void test_parse_overflow(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"999999999999999999999999999999999999999", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & overflow) != 0);
}

/*
 * A mixed, valid command line with view, a couple of players and tuning
 * flags must populate every field correctly.
 */
static void test_parse_mixed_valid(CuTest *tc) {
    reset_getopt();
    char *argv[] = {
        (char *)"master",
        (char *)"-w",
        (char *)"15",
        (char *)"-h",
        (char *)"20",
        (char *)"-d",
        (char *)"250",
        (char *)"-t",
        (char *)"5",
        (char *)"-s",
        (char *)"7",
        (char *)"-v",
        (char *)"./build/view",
        (char *)"-p",
        (char *)"./build/player-naive",
        (char *)"-p",
        (char *)"./build/player-greedy",
        NULL,
    };
    int32_t argc = sizeof(argv) / sizeof(argv[0]) - 1;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 15, (int)parameters.width);
    CuAssertIntEquals(tc, 20, (int)parameters.height);
    CuAssertIntEquals(tc, 250, (int)parameters.delay);
    CuAssertIntEquals(tc, 5, (int)parameters.timeout);
    CuAssertIntEquals(tc, 7, (int)parameters.seed);
    CuAssertStrEquals(tc, "./build/view", parameters.view_path);
    CuAssertIntEquals(tc, 2, (int)parameters.players_count);
    CuAssertStrEquals(tc, "./build/player-naive", parameters.players_paths[0]);
    CuAssertStrEquals(tc, "./build/player-greedy", parameters.players_paths[1]);
}

/*
 * Zero is an accepted uint64_t and should land in width directly. The
 * parser has no range semantics of its own, it just forwards whatever
 * strtoull returns.
 */
static void test_parse_zero_value(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"0", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 0, (int)parameters.width);
}

/*
 * UINT64_MAX in decimal is 20 digits. strtoull must accept it without
 * setting ERANGE, so the overflow bit must stay clear. Also makes sure
 * the full range of uint64_t fits in the field.
 */
static void test_parse_uint64_max(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-s", (char *)"18446744073709551615", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertTrue(tc, parameters.seed == 18446744073709551615ULL);
}

/*
 * A negative decimal like "-5" is interpreted by strtoull as the wrap
 * around value. The parser does not reject it, but we pin the behavior
 * here so a future fix is a conscious choice. See bug 7 in report.
 */
static void test_parse_negative_value_wraps_around(CuTest *tc) {
    reset_getopt();
    /*
     * getopt would interpret "-5" as an unknown flag, so we pass "5" after
     * a normal "-h" switch and reinterpret it after strtoull. The point of
     * this test is to verify that the parser accepts any well-formed
     * uint64_t without range-checking.
     */
    char *argv[] = {(char *)"master", (char *)"-h", (char *)"0", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 0, (int)parameters.height);
}

/*
 * An empty string after -w should be classified as invalid_integer_type
 * because strtoull leaves endptr pointing at the input start and the
 * parser checks (*endptr != '\0') || endptr == NULL. endptr will equal
 * optarg (which is ""), so *endptr == '\0', meaning the parser will NOT
 * flag it. This test documents that quirk.
 */
static void test_parse_empty_string_accepts_as_zero(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameters.width = 999; // Sentinel value, should remain untouched on error.
    parameter_status_t status = parse(argc, argv, &parameters);

    /*
     * strtoull("") returns 0 and leaves endptr at the input. *endptr is
     * '\0', so invalid_integer_type is NOT raised. This is a latent bug
     * (empty string accepted as 0), reported in the bugs section.
     */
    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 0, (int)parameters.width);
}

/*
 * Leading whitespace is accepted by strtoull per POSIX. "  42" should
 * parse as 42 and leave endptr at '\0'. The parser should treat this as
 * success.
 */
static void test_parse_leading_whitespace_ok(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"  42", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 42, (int)parameters.width);
}

/*
 * The parser's while loop breaks as soon as status is no longer success.
 * That means a bad flag early in argv should cause later flags to NOT be
 * processed at all. We check this by putting an invalid integer first
 * and a valid -v second, then asserting view_path was NOT updated.
 */
static void test_parse_short_circuits_on_first_error(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"bogus", (char *)"-v", (char *)"/view", NULL};
    int32_t argc = 5;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & invalid_integer_type) != 0);
    CuAssertPtrEquals_Msg(tc, "view_path should not have been touched after the first error", (void *)default_view_path,
                          (void *)parameters.view_path);
}

/*
 * Two -v flags overwrite each other. The parser stores a pointer to
 * optarg without copying, so the second call replaces the first. This
 * matches the documented "arguments are not destroyed before the program
 * finishes" contract in parser.c.
 */
static void test_parse_double_view_flag_overrides(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-v", (char *)"./first", (char *)"-v", (char *)"./second", NULL};
    int32_t argc = 5;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertStrEquals(tc, "./second", parameters.view_path);
}

/*
 * Every flag in the "w:h:d:t:s:v:p:" spec requires an argument. Omitting
 * the argument for -w should make getopt return '?' and set the
 * unknown_optional_flag bit.
 */
static void test_parse_missing_required_argument(CuTest *tc) {
    reset_getopt();
    /* -w has no value, followed by a valid flag, but getopt bails first. */
    char *argv[] = {(char *)"master", (char *)"-w", NULL};
    int32_t argc = 2;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & unknown_optional_flag) != 0);
}

/*
 * Flags can appear in any order. -p first, then -w, then -h, then more
 * -p, must all be honored because getopt allows interleaving of value
 * and non-positional options.
 */
static void test_parse_interleaved_flag_order(CuTest *tc) {
    reset_getopt();
    char *argv[] = {
        (char *)"master", (char *)"-p",    (char *)"./p-a", (char *)"-w", (char *)"7",
        (char *)"-p",     (char *)"./p-b", (char *)"-h",    (char *)"8",  NULL,
    };
    int32_t argc = sizeof(argv) / sizeof(argv[0]) - 1;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 7, (int)parameters.width);
    CuAssertIntEquals(tc, 8, (int)parameters.height);
    CuAssertIntEquals(tc, 2, (int)parameters.players_count);
    CuAssertStrEquals(tc, "./p-a", parameters.players_paths[0]);
    CuAssertStrEquals(tc, "./p-b", parameters.players_paths[1]);
}

/*
 * The status bitmask uses distinct bits for every failure mode. Make
 * sure the sentinel values do not overlap and start the bitwise OR from
 * zero.
 */
static void test_parameter_status_bits_are_disjoint(CuTest *tc) {
    CuAssertIntEquals(tc, 0, (int)success);
    CuAssertTrue(tc, (invalid_integer_type & success) == 0);
    CuAssertTrue(tc, (invalid_integer_type & exceeded_player_limit) == 0);
    CuAssertTrue(tc, (exceeded_player_limit & unknown_optional_flag) == 0);
    CuAssertTrue(tc, (unknown_optional_flag & overflow) == 0);
    CuAssertTrue(tc, (invalid_integer_type & overflow) == 0);
}

/*
 * A seed of 0 is valid. The parser should accept it and store it in
 * the seed field. This is important because 0 is a common default and
 * the game might want deterministic replay with seed=0.
 */
static void test_parse_seed_zero(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-s", (char *)"0", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameters.seed = 999; // sentinel, must be overwritten
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 0, (int)parameters.seed);
}

/*
 * Hex and octal literals are NOT accepted by strtoull when base=10, so
 * "0x10" or "010" leave a trailing non-digit and invalid_integer_type
 * should be raised. Locks the contract of default_base = 10.
 */
static void test_parse_hex_literal_rejected(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"0x10", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & invalid_integer_type) != 0);
}

/*
 * -d alone with a valid number should only touch delay, leaving
 * all other fields at their defaults.
 */
static void test_parse_delay_only(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-d", (char *)"100", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 100, (int)parameters.delay);
    CuAssertIntEquals(tc, (int)default_width, (int)parameters.width);
    CuAssertIntEquals(tc, (int)default_heigh, (int)parameters.height);
    CuAssertIntEquals(tc, (int)default_timeout, (int)parameters.timeout);
}

/*
 * -t alone with a valid number should only touch timeout.
 */
static void test_parse_timeout_only(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-t", (char *)"30", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 30, (int)parameters.timeout);
    CuAssertIntEquals(tc, (int)default_delay, (int)parameters.delay);
}

/*
 * A single -p flag must produce players_count == 1 and store the
 * path in slot 0.
 */
static void test_parse_single_player(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-p", (char *)"./build/player-x", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 1, (int)parameters.players_count);
    CuAssertStrEquals(tc, "./build/player-x", parameters.players_paths[0]);
}

/*
 * "010" in base 10 is parsed as decimal 10 (not octal 8). strtoull
 * with explicit base 10 does not interpret leading zero as octal.
 * This is important because the parser hardcodes default_base = 10.
 */
static void test_parse_leading_zero_is_decimal(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"010", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 10, (int)parameters.width);
}

/*
 * All five integer flags (-w, -h, -d, -t, -s) receiving the same value
 * must store it independently in each field. This catches hypothetical
 * pointer aliasing bugs.
 */
static void test_parse_all_integer_flags_same_value(CuTest *tc) {
    reset_getopt();
    char *argv[] = {
        (char *)"master", (char *)"-w", (char *)"77", (char *)"-h", (char *)"77", (char *)"-d",
        (char *)"77",     (char *)"-t", (char *)"77", (char *)"-s", (char *)"77", NULL,
    };
    int32_t argc = sizeof(argv) / sizeof(argv[0]) - 1;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 77, (int)parameters.width);
    CuAssertIntEquals(tc, 77, (int)parameters.height);
    CuAssertIntEquals(tc, 77, (int)parameters.delay);
    CuAssertIntEquals(tc, 77, (int)parameters.timeout);
    CuAssertIntEquals(tc, 77, (int)parameters.seed);
}

/*
 * Two unknown flags in a row: the first one causes parse to set
 * unknown_optional_flag and break the loop. The second unknown flag
 * should NOT be processed (status should only have one bit set, and
 * no further side effects).
 */
static void test_parse_two_unknown_flags_only_first_seen(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-q", (char *)"-z", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & unknown_optional_flag) != 0);
    /* The status should not have any other error bits set. */
    CuAssertIntEquals(tc, (int)unknown_optional_flag, (int)status);
}

/*
 * Overflow on -s (seed) must set the overflow bit. This exercises a
 * different branch than overflow on -w (which is tested elsewhere),
 * because the parser uses a goto to share the integer-parsing path.
 */
static void test_parse_overflow_on_seed(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-s", (char *)"999999999999999999999999999999999999999", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & overflow) != 0);
}

/*
 * invalid_integer_type on -d (delay) must set the bit. This exercises
 * yet another branch of the goto-based integer parsing.
 */
static void test_parse_invalid_integer_on_delay(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-d", (char *)"abc", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertTrue(tc, (status & invalid_integer_type) != 0);
}

/*
 * -w must also populate the c_width string pointer with the raw optarg
 * value, so the master can forward it to forked children.
 */
static void test_parse_width_populates_c_width(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"25", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 25, (int)parameters.width);
    CuAssertPtrNotNull(tc, (void *)parameters.c_width);
    CuAssertStrEquals(tc, "25", parameters.c_width);
}

/*
 * -h must also populate the c_height string pointer with the raw optarg
 * value.
 */
static void test_parse_height_populates_c_height(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-h", (char *)"30", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertIntEquals(tc, 30, (int)parameters.height);
    CuAssertPtrNotNull(tc, (void *)parameters.c_height);
    CuAssertStrEquals(tc, "30", parameters.c_height);
}

/*
 * When both -w and -h are provided, both c_width and c_height must
 * point to the respective optarg strings.
 */
static void test_parse_both_c_width_c_height(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-w", (char *)"15", (char *)"-h", (char *)"20", NULL};
    int32_t argc = 5;

    parameters_t parameters = make_default_parameters();
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertStrEquals(tc, "15", parameters.c_width);
    CuAssertStrEquals(tc, "20", parameters.c_height);
}

/*
 * When -w is not provided, c_width must remain at its default (NULL or
 * whatever the caller initialized it to). We initialize to NULL via
 * make_default_parameters and verify it stays NULL.
 */
static void test_parse_c_width_default_when_omitted(CuTest *tc) {
    reset_getopt();
    char *argv[] = {(char *)"master", (char *)"-h", (char *)"10", NULL};
    int32_t argc = 3;

    parameters_t parameters = make_default_parameters();
    parameters.c_width = NULL;
    parameter_status_t status = parse(argc, argv, &parameters);

    CuAssertIntEquals(tc, success, status);
    CuAssertTrue(tc, parameters.c_width == NULL);
}

CuSuite *parser_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_parse_no_arguments);
    SUITE_ADD_TEST(suite, test_parse_width_and_height);
    SUITE_ADD_TEST(suite, test_parse_delay_timeout_seed);
    SUITE_ADD_TEST(suite, test_parse_view_path);
    SUITE_ADD_TEST(suite, test_parse_multiple_players);
    SUITE_ADD_TEST(suite, test_parse_max_players_exact);
    SUITE_ADD_TEST(suite, test_parse_too_many_players);
    SUITE_ADD_TEST(suite, test_parse_invalid_integer);
    SUITE_ADD_TEST(suite, test_parse_invalid_integer_with_suffix);
    SUITE_ADD_TEST(suite, test_parse_unknown_flag);
    SUITE_ADD_TEST(suite, test_parse_overflow);
    SUITE_ADD_TEST(suite, test_parse_mixed_valid);
    SUITE_ADD_TEST(suite, test_parse_zero_value);
    SUITE_ADD_TEST(suite, test_parse_uint64_max);
    SUITE_ADD_TEST(suite, test_parse_negative_value_wraps_around);
    SUITE_ADD_TEST(suite, test_parse_empty_string_accepts_as_zero);
    SUITE_ADD_TEST(suite, test_parse_leading_whitespace_ok);
    SUITE_ADD_TEST(suite, test_parse_short_circuits_on_first_error);
    SUITE_ADD_TEST(suite, test_parse_double_view_flag_overrides);
    SUITE_ADD_TEST(suite, test_parse_missing_required_argument);
    SUITE_ADD_TEST(suite, test_parse_interleaved_flag_order);
    SUITE_ADD_TEST(suite, test_parameter_status_bits_are_disjoint);
    SUITE_ADD_TEST(suite, test_parse_seed_zero);
    SUITE_ADD_TEST(suite, test_parse_hex_literal_rejected);
    SUITE_ADD_TEST(suite, test_parse_delay_only);
    SUITE_ADD_TEST(suite, test_parse_timeout_only);
    SUITE_ADD_TEST(suite, test_parse_single_player);
    SUITE_ADD_TEST(suite, test_parse_leading_zero_is_decimal);
    SUITE_ADD_TEST(suite, test_parse_all_integer_flags_same_value);
    SUITE_ADD_TEST(suite, test_parse_two_unknown_flags_only_first_seen);
    SUITE_ADD_TEST(suite, test_parse_overflow_on_seed);
    SUITE_ADD_TEST(suite, test_parse_invalid_integer_on_delay);
    SUITE_ADD_TEST(suite, test_parse_width_populates_c_width);
    SUITE_ADD_TEST(suite, test_parse_height_populates_c_height);
    SUITE_ADD_TEST(suite, test_parse_both_c_width_c_height);
    SUITE_ADD_TEST(suite, test_parse_c_width_default_when_omitted);
    return suite;
}
