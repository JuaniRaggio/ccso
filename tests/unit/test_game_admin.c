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
 * game_admin exposes the following functions:
 *
 *   - game_register_player: copies a single registration request into a
 *     player_t slot, with bounds and empty-name validation.
 *   - game_register_all: iterates over MAX_PLAYERS registration requests
 *     and counts how many ended up in the array.
 *   - game_state_init: seeds the PRNG and fills the board with reward
 *     values in [1, 9].
 *   - is_move_allowed: checks if a (row, col) coordinate is within the
 *     board and the cell is not already claimed (board[i] >= 0).
 *   - apply_move: if the move is allowed, stamps -player_id on the cell;
 *     otherwise leaves the board untouched. Either way delegates to
 *     register_move to bump the valid/invalid counter.
 *   - register_move: increments the valid_moves or invalid_moves counter
 *     of the player at state->players[player_id].
 *
 * All are pure functions over already-allocated memory, so we can
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
    CuAssertIntEquals(tc, 0, (int)players[0].x);
    CuAssertIntEquals(tc, 0, (int)players[0].y);
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

/* ---------- is_move_allowed ---------- */

/*
 * Helper: make a game, init its board with a fixed seed, and return it.
 * The caller must free_game the result.
 */
static game_t *make_initialized_game(uint16_t width, uint16_t height, uint64_t seed, int8_t players) {
    game_t *game = make_game(width, height);
    if (game == NULL) {
        return NULL;
    }
    game_state_init(game, width, height, seed, players);
    return game;
}

/*
 * A freshly initialized board has all cells in [1, 9], so any in-bounds
 * coordinate must be allowed.
 */
static void test_is_move_allowed_in_bounds_positive_cell(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 1, 1);
    CuAssertPtrNotNull(tc, game);

    CuAssertTrue(tc, is_move_allowed(game->state, 0, 0));
    CuAssertTrue(tc, is_move_allowed(game->state, 2, 3));
    CuAssertTrue(tc, is_move_allowed(game->state, 4, 4));

    free_game(game);
}

/*
 * Any coordinate with vertical_coord >= height must be rejected.
 */
static void test_is_move_allowed_out_of_bounds_vertical(CuTest *tc) {
    game_t *game = make_initialized_game(4, 3, 2, 1);
    CuAssertPtrNotNull(tc, game);

    CuAssertTrue(tc, !is_move_allowed(game->state, 3, 0));
    CuAssertTrue(tc, !is_move_allowed(game->state, 100, 0));

    free_game(game);
}

/*
 * Any coordinate with horizontal_coord >= width must be rejected.
 */
static void test_is_move_allowed_out_of_bounds_horizontal(CuTest *tc) {
    game_t *game = make_initialized_game(4, 3, 3, 1);
    CuAssertPtrNotNull(tc, game);

    CuAssertTrue(tc, !is_move_allowed(game->state, 0, 4));
    CuAssertTrue(tc, !is_move_allowed(game->state, 0, 255));

    free_game(game);
}

/*
 * Both coordinates out of range at the same time must still be rejected.
 */
static void test_is_move_allowed_both_out_of_bounds(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 4, 1);
    CuAssertPtrNotNull(tc, game);

    CuAssertTrue(tc, !is_move_allowed(game->state, 3, 3));
    CuAssertTrue(tc, !is_move_allowed(game->state, 10, 10));

    free_game(game);
}

/*
 * A cell that has been claimed (board value < 0) must not be allowed.
 * We manually stamp a negative value to simulate a prior apply_move.
 */
static void test_is_move_allowed_already_claimed_cell(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 5, 2);
    CuAssertPtrNotNull(tc, game);

    /* Stamp cell (1, 2) as claimed by player 1. */
    game->state->board[1 * 5 + 2] = -1;
    CuAssertTrue(tc, !is_move_allowed(game->state, 1, 2));

    /* Adjacent cell should still be allowed. */
    CuAssertTrue(tc, is_move_allowed(game->state, 1, 3));

    free_game(game);
}

/*
 * A cell with board value 0 means it was captured by player 0
 * (-0 == 0). With the <= 0 fix, is_move_allowed correctly rejects it.
 */
static void test_is_move_allowed_zero_cell_is_captured(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 6, 1);
    CuAssertPtrNotNull(tc, game);

    game->state->board[0] = 0;
    CuAssertTrue(tc, !is_move_allowed(game->state, 0, 0));

    free_game(game);
}

/*
 * The four corners of the board must all be reachable as valid moves
 * on a freshly initialized board.
 */
static void test_is_move_allowed_all_corners(CuTest *tc) {
    const uint16_t w = 6;
    const uint16_t h = 4;
    game_t *game = make_initialized_game(w, h, 7, 1);
    CuAssertPtrNotNull(tc, game);

    CuAssertTrue(tc, is_move_allowed(game->state, 0, 0));
    CuAssertTrue(tc, is_move_allowed(game->state, 0, w - 1));
    CuAssertTrue(tc, is_move_allowed(game->state, h - 1, 0));
    CuAssertTrue(tc, is_move_allowed(game->state, h - 1, w - 1));

    free_game(game);
}

/* ---------- apply_move ---------- */

/*
 * A valid apply_move must stamp -player_id on the target cell and
 * increment the player's valid_moves counter.
 *
 * IMPORTANT: register_move uses player_id as a direct index into
 * state->players[]. In the real game, player_id is a pid_t (process ID),
 * which is usually a large number. Using it as an array index is a bug
 * (documented in the report). In tests we work around this by using
 * small player_id values (0..MAX_PLAYERS-1) that happen to be valid
 * indices.
 */
static void test_apply_move_valid_stamps_cell(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 10, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const int8_t pid = 1;
    apply_move(game->state, 2, 3, pid);

    CuAssertIntEquals(tc, -pid, (int)game->state->board[2 * 5 + 3]);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].valid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[pid].invalid_moves);

    free_game(game);
}

/*
 * An out-of-bounds move must NOT modify any cell. The player's
 * invalid_moves counter must be bumped instead.
 */
static void test_apply_move_out_of_bounds_increments_invalid(CuTest *tc) {
    game_t *game = make_initialized_game(4, 4, 11, 1);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    /* Save the entire board to verify nothing changed. */
    int8_t board_copy[16];
    memcpy(board_copy, game->state->board, 16);

    const int8_t pid = 1;
    apply_move(game->state, 4, 0, pid); /* row 4 is out of bounds on a 4x4 board */

    CuAssertIntEquals(tc, 0, (int)game->state->players[pid].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].invalid_moves);
    CuAssertTrue(tc, memcmp(board_copy, game->state->board, 16) == 0);

    free_game(game);
}

/*
 * Trying to move onto an already-claimed cell must be treated as
 * invalid: board stays unchanged, invalid_moves incremented.
 *
 * NOTE: player_id = 0 cannot claim cells because -0 == 0 and
 * is_move_allowed checks board[i] < 0. We use player IDs >= 1
 * to avoid this edge case. The player-0 issue is documented as
 * a bug in the report.
 */
static void test_apply_move_already_claimed_is_invalid(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 12, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const int8_t pid0 = 1;
    const int8_t pid1 = 2;

    /* First move by player 1 should succeed. */
    apply_move(game->state, 1, 1, pid0);
    CuAssertIntEquals(tc, -pid0, (int)game->state->board[1 * 5 + 1]);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid0].valid_moves);

    /* Second move by player 2 to the same cell should fail. */
    apply_move(game->state, 1, 1, pid1);
    CuAssertIntEquals(tc, -pid0, (int)game->state->board[1 * 5 + 1]); /* unchanged */
    CuAssertIntEquals(tc, 0, (int)game->state->players[pid1].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid1].invalid_moves);

    free_game(game);
}

/*
 * Moving twice to the same cell with the same player: the second move
 * must be invalid because the cell is now negative (holds -player_id).
 * We use player_id >= 1 to avoid the -0 == 0 edge case.
 */
static void test_apply_move_same_cell_twice_is_invalid(CuTest *tc) {
    game_t *game = make_initialized_game(4, 4, 13, 1);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const int8_t pid = 1;
    apply_move(game->state, 0, 0, pid);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].valid_moves);

    apply_move(game->state, 0, 0, pid);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].valid_moves);   /* no change */
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].invalid_moves); /* bumped */

    free_game(game);
}

/*
 * Multiple valid moves by the same player to distinct cells should
 * accumulate valid_moves correctly and stamp each cell.
 */
static void test_apply_move_multiple_valid_moves(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 14, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const int8_t pid = 1;
    apply_move(game->state, 0, 0, pid);
    apply_move(game->state, 0, 1, pid);
    apply_move(game->state, 0, 2, pid);

    CuAssertIntEquals(tc, 3, (int)game->state->players[pid].valid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[pid].invalid_moves);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[0]);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[1]);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[2]);

    free_game(game);
}

/*
 * Moves at the four corner cells of the board must all be valid on a
 * fresh board and must stamp the correct offset.
 */
static void test_apply_move_corner_cells(CuTest *tc) {
    const uint16_t w = 6;
    const uint16_t h = 4;
    game_t *game = make_initialized_game(w, h, 15, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const int8_t pid = 1;
    apply_move(game->state, 0, 0, pid);
    apply_move(game->state, 0, w - 1, pid);
    apply_move(game->state, h - 1, 0, pid);
    apply_move(game->state, h - 1, w - 1, pid);

    CuAssertIntEquals(tc, 4, (int)game->state->players[pid].valid_moves);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[0]);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[w - 1]);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[(h - 1) * w]);
    CuAssertIntEquals(tc, -pid, (int)game->state->board[(h - 1) * w + (w - 1)]);

    free_game(game);
}

/*
 * Player 0 claims cells correctly: -0 == 0 and is_move_allowed
 * rejects cells with board[i] <= 0, so the cell is properly captured.
 * A second move to the same cell is correctly rejected as invalid.
 */
static void test_apply_move_player_zero_claims_correctly(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 16, 1);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const int8_t pid = 0;
    apply_move(game->state, 0, 0, pid);
    /* The cell is stamped as -0 = 0 */
    CuAssertIntEquals(tc, 0, (int)game->state->board[0]);
    /* is_move_allowed correctly rejects it (0 <= 0) */
    CuAssertTrue(tc, !is_move_allowed(game->state, 0, 0));
    /* Second move to same cell is invalid */
    apply_move(game->state, 0, 0, pid);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[pid].invalid_moves);

    free_game(game);
}

/* ---------- register_move ---------- */

/*
 * register_move with is_valid_move=true must bump valid_moves.
 */
static void test_register_move_valid(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 20, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    register_move(game->state, true, 0);
    register_move(game->state, true, 0);
    register_move(game->state, true, 0);

    CuAssertIntEquals(tc, 3, (int)game->state->players[0].valid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[0].invalid_moves);

    free_game(game);
}

/*
 * register_move with is_valid_move=false must bump invalid_moves.
 */
static void test_register_move_invalid(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 21, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    register_move(game->state, false, 1);
    register_move(game->state, false, 1);

    CuAssertIntEquals(tc, 0, (int)game->state->players[1].valid_moves);
    CuAssertIntEquals(tc, 2, (int)game->state->players[1].invalid_moves);

    free_game(game);
}

/*
 * register_move for different player IDs must only touch their
 * respective counters and not interfere with each other.
 */
static void test_register_move_different_players_isolated(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 22, 3);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    register_move(game->state, true, 0);
    register_move(game->state, false, 1);
    register_move(game->state, true, 2);
    register_move(game->state, true, 2);

    CuAssertIntEquals(tc, 1, (int)game->state->players[0].valid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[0].invalid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[1].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[1].invalid_moves);
    CuAssertIntEquals(tc, 2, (int)game->state->players[2].valid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[2].invalid_moves);

    free_game(game);
}

/*
 * Interleaving valid and invalid moves for a single player must
 * accumulate both counters independently.
 */
static void test_register_move_interleaved(CuTest *tc) {
    game_t *game = make_initialized_game(3, 3, 23, 1);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    register_move(game->state, true, 0);
    register_move(game->state, false, 0);
    register_move(game->state, true, 0);
    register_move(game->state, false, 0);
    register_move(game->state, true, 0);

    CuAssertIntEquals(tc, 3, (int)game->state->players[0].valid_moves);
    CuAssertIntEquals(tc, 2, (int)game->state->players[0].invalid_moves);

    free_game(game);
}

/* ---------- process_player_move ---------- */

/*
 * process_player_move combines direction validation, coordinate
 * computation via apply_direction, and apply_move into a single
 * function. It operates on the player's current (x, y) position
 * stored in state->players[player_idx].
 */

/*
 * A valid direction on a fresh board: the player starts at (2, 2) and
 * moves right. The new position should be (3, 2), and the cell at
 * row=2, col=3 must be stamped with -player_idx.
 */
static void test_process_player_move_valid_direction(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 30, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const uint8_t idx = 1;
    game->state->players[idx].x = 2;
    game->state->players[idx].y = 2;

    process_player_move(game->state, idx, dir_right);

    /* apply_direction(2, 2, dir_right) -> new_x=3, new_y=2 */
    /* apply_move(state, new_y=2, new_x=3, player_idx=1) */
    CuAssertIntEquals(tc, -(int8_t)idx, (int)game->state->board[2 * 5 + 3]);
    CuAssertIntEquals(tc, 1, (int)game->state->players[idx].valid_moves);
    CuAssertIntEquals(tc, 0, (int)game->state->players[idx].invalid_moves);

    free_game(game);
}

/*
 * An invalid direction (e.g. NO_VALID_MOVE = -1) must be rejected
 * and increment the player's invalid_moves counter without touching
 * the board.
 */
static void test_process_player_move_invalid_direction(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 31, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    /* Save board for comparison. */
    int8_t board_copy[25];
    memcpy(board_copy, game->state->board, 25);

    const uint8_t idx = 1;
    game->state->players[idx].x = 2;
    game->state->players[idx].y = 2;

    process_player_move(game->state, idx, NO_VALID_MOVE);

    CuAssertIntEquals(tc, 0, (int)game->state->players[idx].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[idx].invalid_moves);
    CuAssertTrue(tc, memcmp(board_copy, game->state->board, 25) == 0);

    free_game(game);
}

/*
 * A valid direction that leads out of bounds (player at (0, 0) moving
 * up means new_y=-1). The apply_move call receives uint16_t coordinates,
 * and -1 cast to uint16_t is 65535 which is out of bounds. So the move
 * should be invalid.
 */
static void test_process_player_move_out_of_bounds_wraps(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 32, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const uint8_t idx = 1;
    game->state->players[idx].x = 0;
    game->state->players[idx].y = 0;

    process_player_move(game->state, idx, dir_up);

    /* dir_up: new_y = 0 + (-1) = -1, passed to apply_move as uint16_t = 65535 */
    CuAssertIntEquals(tc, 0, (int)game->state->players[idx].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[idx].invalid_moves);

    free_game(game);
}

/*
 * All 8 directions from the center of a 5x5 board must be valid moves.
 * After each move the target cell must be stamped.
 */
static void test_process_player_move_all_directions_from_center(CuTest *tc) {
    const uint8_t idx = 1;
    /*
     * For each direction, create a fresh game, place the player at center
     * (2,2) and try the move. We use separate games to avoid interference
     * between moves (each claimed cell blocks the next).
     */
    for (int8_t d = 0; d < (int8_t)dir_count; d++) {
        game_t *g = make_initialized_game(5, 5, 33 + d, 2);
        CuAssertPtrNotNull(tc, g);
        memset(g->state->players, 0, sizeof(player_t) * MAX_PLAYERS);
        g->state->players[idx].x = 2;
        g->state->players[idx].y = 2;

        process_player_move(g->state, idx, d);
        CuAssertIntEquals(tc, 1, (int)g->state->players[idx].valid_moves);
        CuAssertIntEquals(tc, 0, (int)g->state->players[idx].invalid_moves);

        free_game(g);
    }
}

/*
 * process_player_move with dir_count (= 8) should be rejected as
 * invalid since is_valid_direction returns false for dir_count.
 */
static void test_process_player_move_dir_count_is_invalid(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 34, 2);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    const uint8_t idx = 1;
    game->state->players[idx].x = 2;
    game->state->players[idx].y = 2;

    process_player_move(game->state, idx, (direction_wire_t)dir_count);

    CuAssertIntEquals(tc, 0, (int)game->state->players[idx].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[idx].invalid_moves);

    free_game(game);
}

/*
 * Moving to an already-claimed cell through process_player_move must
 * still be treated as invalid (apply_move handles this).
 */
static void test_process_player_move_to_claimed_cell(CuTest *tc) {
    game_t *game = make_initialized_game(5, 5, 35, 3);
    CuAssertPtrNotNull(tc, game);
    memset(game->state->players, 0, sizeof(player_t) * MAX_PLAYERS);

    /* Manually claim cell (2, 3) = row 2, col 3 */
    game->state->board[2 * 5 + 3] = -2;

    const uint8_t idx = 1;
    game->state->players[idx].x = 2;
    game->state->players[idx].y = 2;

    /* dir_right: new_x=3, new_y=2 -> cell (2,3) is already claimed */
    process_player_move(game->state, idx, dir_right);

    CuAssertIntEquals(tc, 0, (int)game->state->players[idx].valid_moves);
    CuAssertIntEquals(tc, 1, (int)game->state->players[idx].invalid_moves);
    CuAssertIntEquals(tc, -2, (int)game->state->board[2 * 5 + 3]); /* unchanged */

    free_game(game);
}

/* ---------- game_state_init additional tests ---------- */

/*
 * Different seeds must produce different boards (with overwhelming
 * probability). We initialize two boards with distinct seeds and check
 * that at least one cell differs.
 */
static void test_game_state_init_different_seeds_differ(CuTest *tc) {
    const uint16_t w = 8;
    const uint16_t h = 8;

    game_t *a = make_game(w, h);
    game_t *b = make_game(w, h);
    CuAssertPtrNotNull(tc, a);
    CuAssertPtrNotNull(tc, b);

    game_state_init(a, w, h, 100, 2);
    game_state_init(b, w, h, 200, 2);

    bool found_diff = false;
    for (int32_t i = 0; i < w * h; i++) {
        if (a->state->board[i] != b->state->board[i]) {
            found_diff = true;
            break;
        }
    }
    CuAssertTrue(tc, found_diff);

    free_game(a);
    free_game(b);
}

/*
 * A large board (e.g. 100x100) must still have all cells in [1, 9].
 * This is a stress/bounds test for the board_init loop.
 */
static void test_game_state_init_large_board(CuTest *tc) {
    const uint16_t w = 100;
    const uint16_t h = 100;

    game_t *game = make_game(w, h);
    CuAssertPtrNotNull(tc, game);

    game_state_init(game, w, h, 42, 4);

    CuAssertIntEquals(tc, w, (int)game->state->width);
    CuAssertIntEquals(tc, h, (int)game->state->height);
    for (int32_t i = 0; i < w * h; i++) {
        CuAssertTrue(tc, game->state->board[i] >= 1 && game->state->board[i] <= 9);
    }

    free_game(game);
}

CuSuite *game_admin_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    /* game_register_player */
    SUITE_ADD_TEST(suite, test_register_player_happy_path);
    SUITE_ADD_TEST(suite, test_register_player_rejects_empty_name);
    SUITE_ADD_TEST(suite, test_register_player_rejects_out_of_range_idx);
    SUITE_ADD_TEST(suite, test_register_player_max_length_name);
    SUITE_ADD_TEST(suite, test_register_player_truncates_long_name);
    SUITE_ADD_TEST(suite, test_register_player_overrides_existing_slot);
    SUITE_ADD_TEST(suite, test_register_player_last_slot);
    /* game_register_all */
    SUITE_ADD_TEST(suite, test_register_all_null_input_returns_without_writing);
    SUITE_ADD_TEST(suite, test_register_all_fills_every_slot);
    SUITE_ADD_TEST(suite, test_register_all_skips_empty_names);
    SUITE_ADD_TEST(suite, test_register_all_empty_set_returns_zero);
    /* game_state_init */
    SUITE_ADD_TEST(suite, test_game_state_init_populates_board);
    SUITE_ADD_TEST(suite, test_game_state_init_same_seed_produces_same_board);
    SUITE_ADD_TEST(suite, test_game_state_init_minimum_board);
    SUITE_ADD_TEST(suite, test_game_state_init_resets_running_flag);
    SUITE_ADD_TEST(suite, test_game_state_init_different_seeds_differ);
    SUITE_ADD_TEST(suite, test_game_state_init_large_board);
    /* is_move_allowed */
    SUITE_ADD_TEST(suite, test_is_move_allowed_in_bounds_positive_cell);
    SUITE_ADD_TEST(suite, test_is_move_allowed_out_of_bounds_vertical);
    SUITE_ADD_TEST(suite, test_is_move_allowed_out_of_bounds_horizontal);
    SUITE_ADD_TEST(suite, test_is_move_allowed_both_out_of_bounds);
    SUITE_ADD_TEST(suite, test_is_move_allowed_already_claimed_cell);
    SUITE_ADD_TEST(suite, test_is_move_allowed_zero_cell_is_captured);
    SUITE_ADD_TEST(suite, test_is_move_allowed_all_corners);
    /* apply_move */
    SUITE_ADD_TEST(suite, test_apply_move_valid_stamps_cell);
    SUITE_ADD_TEST(suite, test_apply_move_out_of_bounds_increments_invalid);
    SUITE_ADD_TEST(suite, test_apply_move_already_claimed_is_invalid);
    SUITE_ADD_TEST(suite, test_apply_move_same_cell_twice_is_invalid);
    SUITE_ADD_TEST(suite, test_apply_move_multiple_valid_moves);
    SUITE_ADD_TEST(suite, test_apply_move_corner_cells);
    SUITE_ADD_TEST(suite, test_apply_move_player_zero_claims_correctly);
    /* register_move */
    SUITE_ADD_TEST(suite, test_register_move_valid);
    SUITE_ADD_TEST(suite, test_register_move_invalid);
    SUITE_ADD_TEST(suite, test_register_move_different_players_isolated);
    SUITE_ADD_TEST(suite, test_register_move_interleaved);
    /* process_player_move */
    SUITE_ADD_TEST(suite, test_process_player_move_valid_direction);
    SUITE_ADD_TEST(suite, test_process_player_move_invalid_direction);
    SUITE_ADD_TEST(suite, test_process_player_move_out_of_bounds_wraps);
    SUITE_ADD_TEST(suite, test_process_player_move_all_directions_from_center);
    SUITE_ADD_TEST(suite, test_process_player_move_dir_count_is_invalid);
    SUITE_ADD_TEST(suite, test_process_player_move_to_claimed_cell);
    return suite;
}
