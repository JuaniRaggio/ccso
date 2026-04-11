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
    return suite;
}
