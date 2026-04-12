#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "CuTest.h"
#include "test_suites.h"

#include <game.h>
#include <game_admin.h>
#include <game_state.h>

/*
 * game_admin exposes three functions:
 *
 *   - game_register_player: copies a single registration request into a
 *     player_t slot, with bounds and empty-name validation.
 *   - game_register_all: iterates over MAX_PLAYERS registration requests
 *     and counts how many ended up in the array.
 *   - game_state_init: seeds the PRNG and fills the board with reward
 *     values in [1, 9].
 *
 * All three are pure functions over already-allocated memory, so we can
 * exercise them without going through shared memory or shared state.
 *
 * Error paths call manage_error, which prints to stderr. To keep the
 * test output clean we redirect stderr to /dev/null for the duration
 * of error-path tests. The restoration helper runs even if the test
 * body failed, so a test failure cannot permanently swallow stderr.
 */

typedef struct {
    int32_t saved_fd;
    int32_t devnull_fd;
} stderr_silencer_t;

static int32_t stderr_silence_begin(stderr_silencer_t *s) {
    fflush(stderr);
    s->saved_fd = dup(fileno(stderr));
    if (s->saved_fd == -1) {
        return -1;
    }
    s->devnull_fd = open("/dev/null", O_WRONLY);
    if (s->devnull_fd == -1) {
        close(s->saved_fd);
        return -1;
    }
    if (dup2(s->devnull_fd, fileno(stderr)) == -1) {
        close(s->devnull_fd);
        close(s->saved_fd);
        return -1;
    }
    return 0;
}

static void stderr_silence_end(stderr_silencer_t *s) {
    fflush(stderr);
    dup2(s->saved_fd, fileno(stderr));
    close(s->devnull_fd);
    close(s->saved_fd);
}

/* ---------- game_register_player ---------- */

static void clear_players(player_t players[MAX_PLAYERS]) {
    memset(players, 0, sizeof(player_t) * MAX_PLAYERS);
}

/*
 * Happy path: a valid registration must populate the target slot with
 * the provided pid and name, start score/moves at zero, and leave state
 * as alive.
 */
static void test_register_player_happy_path(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t req = {
        .player_pid = 4242,
        .name = "alice",
    };

    bool ok = game_register_player(players, 0, req);
    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 4242, (int)players[0].player_id);
    CuAssertStrEquals(tc, "alice", players[0].name);
    CuAssertIntEquals(tc, 0, (int)players[0].score);
    CuAssertIntEquals(tc, 0, (int)players[0].valid_moves);
    CuAssertIntEquals(tc, 0, (int)players[0].invalid_moves);
    CuAssertTrue(tc, players[0].state);
}

/*
 * An empty name must be rejected with false and the target slot must
 * remain untouched.
 */
static void test_register_player_rejects_empty_name(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t req = {
        .player_pid = 99,
        .name = "",
    };

    stderr_silencer_t silencer;
    CuAssertIntEquals(tc, 0, stderr_silence_begin(&silencer));
    bool ok = game_register_player(players, 0, req);
    stderr_silence_end(&silencer);

    CuAssertTrue(tc, !ok);
    /* The slot must still be zeroed since we rejected the write. */
    CuAssertIntEquals(tc, 0, (int)players[0].player_id);
    CuAssertIntEquals(tc, 0, (int)players[0].name[0]);
}

/*
 * An index past MAX_PLAYERS must also be rejected. We cannot actually
 * verify nothing was written because the write would be out of bounds,
 * but the function should return false before touching memory.
 */
static void test_register_player_rejects_out_of_range_idx(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t req = {
        .player_pid = 1,
        .name = "bob",
    };

    stderr_silencer_t silencer;
    CuAssertIntEquals(tc, 0, stderr_silence_begin(&silencer));
    bool ok = game_register_player(players, MAX_PLAYERS, req);
    stderr_silence_end(&silencer);

    CuAssertTrue(tc, !ok);
    /* Slot 0 must stay empty since the error path did not fall through. */
    CuAssertIntEquals(tc, 0, (int)players[0].player_id);
    CuAssertIntEquals(tc, 0, (int)players[0].name[0]);
}

/*
 * A name that is exactly MAX_NAME_LENGTH - 1 chars must be stored
 * verbatim and the final byte must still be the NUL terminator written
 * explicitly by the implementation (strncpy + manual NUL).
 */
static void test_register_player_max_length_name(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    char name[MAX_NAME_LENGTH];
    for (int32_t i = 0; i < MAX_NAME_LENGTH - 1; i++) {
        name[i] = 'a';
    }
    name[MAX_NAME_LENGTH - 1] = '\0';

    player_registration_requirements_t req = {.player_pid = 10};
    memcpy((char *)req.name, name, MAX_NAME_LENGTH);

    bool ok = game_register_player(players, 1, req);
    CuAssertTrue(tc, ok);
    CuAssertStrEquals(tc, name, players[1].name);
    CuAssertIntEquals(tc, 0, (int)players[1].name[MAX_NAME_LENGTH - 1]);
}

/*
 * A name that is exactly MAX_NAME_LENGTH chars (no trailing NUL fits)
 * must be truncated to MAX_NAME_LENGTH - 1 chars and still be null
 * terminated, per the "name[MAX_NAME_LENGTH - 1] = '\0'" line in
 * game_register_player.
 */
static void test_register_player_truncates_long_name(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    /*
     * Fill the fixed-length name buffer of the registration struct with
     * MAX_NAME_LENGTH 'X' characters (no NUL fits by design). strncpy
     * will copy all of them, then the implementation forces the last
     * byte to NUL, truncating to MAX_NAME_LENGTH - 1 visible chars.
     */
    player_registration_requirements_t req = {.player_pid = 7};
    for (int32_t i = 0; i < MAX_NAME_LENGTH; i++) {
        ((char *)req.name)[i] = 'X';
    }

    bool ok = game_register_player(players, 0, req);
    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 0, (int)players[0].name[MAX_NAME_LENGTH - 1]);
    /* First MAX_NAME_LENGTH - 1 chars must be 'X'. */
    for (int32_t i = 0; i < MAX_NAME_LENGTH - 1; i++) {
        CuAssertIntEquals(tc, 'X', (int)players[0].name[i]);
    }
}

/*
 * The register_player helper explicitly documents that "if a player
 * already exists at the index, we override it". Pin that behavior.
 */
static void test_register_player_overrides_existing_slot(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    /* Prime slot 2 with a junk player that should be wiped. */
    players[2].player_id = 9999;
    players[2].score = 42;
    players[2].valid_moves = 10;
    strncpy(players[2].name, "zombie", MAX_NAME_LENGTH - 1);
    players[2].state = false;

    player_registration_requirements_t req = {
        .player_pid = 123,
        .name = "ghost",
    };

    bool ok = game_register_player(players, 2, req);
    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 123, (int)players[2].player_id);
    CuAssertStrEquals(tc, "ghost", players[2].name);
    CuAssertIntEquals(tc, 0, (int)players[2].score);
    CuAssertIntEquals(tc, 0, (int)players[2].valid_moves);
    CuAssertTrue(tc, players[2].state);
}

/*
 * The last slot (MAX_PLAYERS - 1) is the boundary that bounds checks
 * often get wrong. Exercising it explicitly catches off-by-one bugs.
 */
static void test_register_player_last_slot(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t req = {
        .player_pid = 5555,
        .name = "last",
    };

    bool ok = game_register_player(players, MAX_PLAYERS - 1, req);
    CuAssertTrue(tc, ok);
    CuAssertIntEquals(tc, 5555, (int)players[MAX_PLAYERS - 1].player_id);
    CuAssertStrEquals(tc, "last", players[MAX_PLAYERS - 1].name);
}

/* ---------- game_register_all ---------- */

/*
 * A NULL to_register must return without writing anywhere. The
 * implementation forwards the error code to manage_error, which returns
 * invalid_argument_error, and the function returns that value via an
 * implicit size_t cast. We do not check the exact return value since
 * it's effectively a numeric error code, not a count; we only verify
 * the players array was left untouched.
 */
static void test_register_all_null_input_returns_without_writing(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    /* Prime slot 0 with a sentinel to detect any write. */
    players[0].player_id = 0xDEAD;

    stderr_silencer_t silencer;
    CuAssertIntEquals(tc, 0, stderr_silence_begin(&silencer));
    size_t registered = game_register_all(players, NULL);
    stderr_silence_end(&silencer);

    /* The function returns the error code, which is non zero. */
    CuAssertTrue(tc, registered != 0);
    /* Slot 0 must still contain the sentinel. */
    CuAssertIntEquals(tc, 0xDEAD, (int)players[0].player_id);
}

/*
 * All MAX_PLAYERS slots with valid names must be counted as
 * successfully registered.
 */
static void test_register_all_fills_every_slot(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t reqs[MAX_PLAYERS];
    memset(reqs, 0, sizeof(reqs));
    static const char *const names[MAX_PLAYERS] = {
        "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8",
    };
    for (int32_t i = 0; i < MAX_PLAYERS; i++) {
        reqs[i].player_pid = 1000 + i;
        strncpy((char *)reqs[i].name, names[i], MAX_NAME_LENGTH - 1);
    }

    size_t registered = game_register_all(players, reqs);
    CuAssertIntEquals(tc, MAX_PLAYERS, (int)registered);
    for (int32_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, 1000 + i, (int)players[i].player_id);
        CuAssertStrEquals(tc, names[i], players[i].name);
        CuAssertTrue(tc, players[i].state);
    }
}

/*
 * Mixing valid and empty-name entries should only count the valid ones.
 * Empty-name entries are rejected inside game_register_player and are
 * NOT counted in the total.
 */
static void test_register_all_skips_empty_names(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t reqs[MAX_PLAYERS];
    memset(reqs, 0, sizeof(reqs));
    /* Even indices are valid, odd ones are empty. */
    int32_t expected = 0;
    for (int32_t i = 0; i < MAX_PLAYERS; i++) {
        reqs[i].player_pid = 2000 + i;
        if ((i & 1) == 0) {
            ((char *)reqs[i].name)[0] = (char)('a' + i);
            ((char *)reqs[i].name)[1] = '\0';
            expected++;
        }
    }

    stderr_silencer_t silencer;
    CuAssertIntEquals(tc, 0, stderr_silence_begin(&silencer));
    size_t registered = game_register_all(players, reqs);
    stderr_silence_end(&silencer);

    CuAssertIntEquals(tc, expected, (int)registered);
    /* Verify the even slots were written and the odd ones were left zero. */
    for (int32_t i = 0; i < MAX_PLAYERS; i++) {
        if ((i & 1) == 0) {
            CuAssertIntEquals(tc, 2000 + i, (int)players[i].player_id);
        } else {
            CuAssertIntEquals(tc, 0, (int)players[i].player_id);
        }
    }
}

/*
 * An empty set of requests (all empty names) must produce zero
 * registrations. This covers the "nothing to do" path.
 */
static void test_register_all_empty_set_returns_zero(CuTest *tc) {
    player_t players[MAX_PLAYERS];
    clear_players(players);

    player_registration_requirements_t reqs[MAX_PLAYERS];
    memset(reqs, 0, sizeof(reqs));

    /* Every slot is rejected, so we silence the stderr noise. */
    stderr_silencer_t silencer;
    CuAssertIntEquals(tc, 0, stderr_silence_begin(&silencer));
    size_t registered = game_register_all(players, reqs);
    stderr_silence_end(&silencer);

    CuAssertIntEquals(tc, 0, (int)registered);
}

/* ---------- game_state_init ---------- */

/* Helper: heap-allocate a game_t whose state has room for a width*height board. */
static game_t *make_game(uint16_t width, uint16_t height) {
    game_t *game = calloc(1, sizeof(game_t));
    if (game == NULL) {
        return NULL;
    }
    game->state = calloc(1, sizeof(game_state_t) + (size_t)width * height);
    if (game->state == NULL) {
        free(game);
        return NULL;
    }
    return game;
}

static void free_game(game_t *game) {
    if (game == NULL) {
        return;
    }
    free(game->state);
    free(game);
}

/*
 * game_state_init must fill width, height, players_count, running, and
 * every board cell with a value in [MIN_CELL_REWARD, MAX_CELL_REWARD],
 * which by convention in this codebase is [1, 9].
 */
static void test_game_state_init_populates_board(CuTest *tc) {
    const uint16_t width = 8;
    const uint16_t height = 6;

    game_t *game = make_game(width, height);
    CuAssertPtrNotNull(tc, game);

    game_state_init(game, width, height, /*seed=*/12345, /*players=*/3);

    CuAssertIntEquals(tc, width, (int)game->state->width);
    CuAssertIntEquals(tc, height, (int)game->state->height);
    CuAssertIntEquals(tc, 3, (int)game->state->players_count);
    CuAssertTrue(tc, !game->state->running);

    for (int32_t i = 0; i < width * height; i++) {
        int32_t cell = game->state->board[i];
        CuAssertTrue(tc, cell >= 1 && cell <= 9);
    }

    free_game(game);
}

/*
 * Calling game_state_init twice with the same seed must produce the
 * exact same board layout. This is the property the master relies on
 * for deterministic replay.
 */
static void test_game_state_init_same_seed_produces_same_board(CuTest *tc) {
    const uint16_t width = 5;
    const uint16_t height = 4;
    const uint64_t seed = 0xCAFE;

    game_t *a = make_game(width, height);
    game_t *b = make_game(width, height);
    CuAssertPtrNotNull(tc, a);
    CuAssertPtrNotNull(tc, b);

    game_state_init(a, width, height, seed, 2);
    game_state_init(b, width, height, seed, 2);

    for (int32_t i = 0; i < width * height; i++) {
        CuAssertIntEquals(tc, a->state->board[i], b->state->board[i]);
    }

    free_game(a);
    free_game(b);
}

/*
 * A 1x1 board is the smallest meaningful input. It must still be filled
 * with exactly one reward cell in [1, 9] and the dimensions must be
 * set accordingly.
 */
static void test_game_state_init_minimum_board(CuTest *tc) {
    const uint16_t width = 1;
    const uint16_t height = 1;

    game_t *game = make_game(width, height);
    CuAssertPtrNotNull(tc, game);

    game_state_init(game, width, height, 0, 1);

    CuAssertIntEquals(tc, 1, (int)game->state->width);
    CuAssertIntEquals(tc, 1, (int)game->state->height);
    CuAssertTrue(tc, game->state->board[0] >= 1 && game->state->board[0] <= 9);

    free_game(game);
}

/*
 * running must always be reset to false regardless of whatever value
 * it had before. The master is supposed to flip it to true only once
 * every player has been registered.
 */
static void test_game_state_init_resets_running_flag(CuTest *tc) {
    const uint16_t width = 3;
    const uint16_t height = 3;

    game_t *game = make_game(width, height);
    CuAssertPtrNotNull(tc, game);

    /* Prime with running = true to prove init resets it. */
    game->state->running = true;
    game_state_init(game, width, height, 1, 1);
    CuAssertTrue(tc, !game->state->running);

    free_game(game);
}

CuSuite *game_admin_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_register_player_happy_path);
    SUITE_ADD_TEST(suite, test_register_player_rejects_empty_name);
    SUITE_ADD_TEST(suite, test_register_player_rejects_out_of_range_idx);
    SUITE_ADD_TEST(suite, test_register_player_max_length_name);
    SUITE_ADD_TEST(suite, test_register_player_truncates_long_name);
    SUITE_ADD_TEST(suite, test_register_player_overrides_existing_slot);
    SUITE_ADD_TEST(suite, test_register_player_last_slot);
    SUITE_ADD_TEST(suite, test_register_all_null_input_returns_without_writing);
    SUITE_ADD_TEST(suite, test_register_all_fills_every_slot);
    SUITE_ADD_TEST(suite, test_register_all_skips_empty_names);
    SUITE_ADD_TEST(suite, test_register_all_empty_set_returns_zero);
    SUITE_ADD_TEST(suite, test_game_state_init_populates_board);
    SUITE_ADD_TEST(suite, test_game_state_init_same_seed_produces_same_board);
    SUITE_ADD_TEST(suite, test_game_state_init_minimum_board);
    SUITE_ADD_TEST(suite, test_game_state_init_resets_running_flag);
    return suite;
}
