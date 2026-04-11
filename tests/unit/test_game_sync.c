#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "test_suites.h"

#include <game_state.h>
#include <game_sync.h>

/*
 * game_sync_t lives in POSIX shared memory at runtime, but its fields are
 * plain in-process unnamed semaphores. On Linux (where Docker runs these
 * tests) sem_init with pshared=1 on a regular heap allocation works just
 * fine for single-process usage, which is exactly what we need here. All
 * assertions stay inside one thread, so every sem_wait/sem_post pair
 * returns immediately as long as the initial counters are set correctly.
 */

/* Helper: allocate a zeroed game_sync_t on the heap and initialize it. */
static game_sync_t *make_sync(void) {
    game_sync_t *sync = calloc(1, sizeof(game_sync_t));
    if (sync == NULL) {
        return NULL;
    }
    game_sync_init(sync);
    return sync;
}

/* Helper: destroy and free a game_sync_t created via make_sync. */
static void free_sync(game_sync_t *sync) {
    if (sync == NULL) {
        return;
    }
    game_sync_destroy(sync);
    free(sync);
}

/* Helper: read a semaphore value without blocking. */
static int32_t sem_value(sem_t *sem) {
    int32_t value = -1;
    sem_getvalue(sem, &value);
    return value;
}

/*
 * game_sync_init must leave the struct in the exact state described by the
 * comments in game_sync.c: view semaphores locked, writer/mutex unlocked,
 * reader count at zero, and every per-player turn gate locked.
 */
static void test_init_sets_expected_initial_values(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_may_render));
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_rendered));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->readers_count_mutex));
    CuAssertIntEquals(tc, 0, (int)sync->readers_count);

    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals_Msg(tc, "player turn gate must start locked", (int)SEM_LOCKED,
                              sem_value(&sync->player_may_send_movement[i]));
    }

    free_sync(sync);
}

/*
 * A writer_enter followed by writer_exit must leave every touched
 * semaphore back at its initial unlocked value. This is the happy path
 * used by the master between frames.
 */
static void test_writer_enter_exit_is_balanced(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    game_sync_writer_enter(sync);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));

    game_sync_writer_exit(sync);
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));

    free_sync(sync);
}

/*
 * A single reader_enter followed by reader_exit must leave readers_count
 * back at zero and release the gamestate mutex so a writer can enter right
 * after.
 */
static void test_single_reader_enter_exit_is_balanced(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    game_sync_reader_enter(sync);
    CuAssertIntEquals(tc, 1, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->readers_count_mutex));

    game_sync_reader_exit(sync);
    CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));

    free_sync(sync);
}

/*
 * With multiple nested readers, only the first one grabs the gamestate
 * mutex. Subsequent ones just bump readers_count. The gamestate mutex is
 * only released when the last reader exits.
 */
static void test_multiple_readers_only_first_takes_mutex(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    game_sync_reader_enter(sync);
    CuAssertIntEquals(tc, 1, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));

    game_sync_reader_enter(sync);
    CuAssertIntEquals(tc, 2, (int)sync->readers_count);
    CuAssertIntEquals_Msg(tc, "second reader must not touch gamestate mutex", (int)SEM_LOCKED,
                          sem_value(&sync->gamestate_mutex));

    game_sync_reader_enter(sync);
    CuAssertIntEquals(tc, 3, (int)sync->readers_count);

    game_sync_reader_exit(sync);
    CuAssertIntEquals(tc, 2, (int)sync->readers_count);
    CuAssertIntEquals_Msg(tc, "mutex released only by last reader", (int)SEM_LOCKED,
                          sem_value(&sync->gamestate_mutex));

    game_sync_reader_exit(sync);
    CuAssertIntEquals(tc, 1, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));

    game_sync_reader_exit(sync);
    CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    CuAssertIntEquals_Msg(tc, "last reader must release gamestate mutex", (int)SEM_UNLOCKED,
                          sem_value(&sync->gamestate_mutex));

    free_sync(sync);
}

/*
 * notify_view posts on view_may_render, and view_wait_frame is just the
 * symmetric wait. Calling them in sequence must leave the semaphore back
 * at SEM_LOCKED.
 */
static void test_view_may_render_post_wait_cycle(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    game_sync_notify_view(sync);
    CuAssertIntEquals(tc, 1, sem_value(&sync->view_may_render));

    game_sync_view_wait_frame(sync);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_may_render));

    free_sync(sync);
}

/*
 * view_frame_done posts view_rendered and wait_view_done consumes it. The
 * master uses this to block until the view has drawn the current frame.
 */
static void test_view_rendered_post_wait_cycle(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    game_sync_view_frame_done(sync);
    CuAssertIntEquals(tc, 1, sem_value(&sync->view_rendered));

    game_sync_wait_view_done(sync);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_rendered));

    free_sync(sync);
}

/*
 * Granting a turn to a specific player must only touch that player's slot,
 * never the other ones. After the player waits for its turn, the slot
 * goes back to SEM_LOCKED.
 */
static void test_player_turn_grant_is_isolated(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    const uint8_t target = 3;
    game_sync_player_grant_turn(sync, target);

    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        int32_t expected = i == target ? 1 : (int32_t)SEM_LOCKED;
        CuAssertIntEquals_Msg(tc, "only the target player slot must be posted", (int)expected,
                              sem_value(&sync->player_may_send_movement[i]));
    }

    game_sync_player_wait_turn(sync, target);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[target]));

    free_sync(sync);
}

/*
 * Granting a turn to every player in sequence must leave all slots with
 * value 1 simultaneously, and then a single wait per slot must drain
 * them all back to locked. This confirms the per-player array is wired
 * correctly across all MAX_PLAYERS indices.
 */
static void test_player_turn_grant_all_players(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        game_sync_player_grant_turn(sync, i);
    }
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, 1, sem_value(&sync->player_may_send_movement[i]));
    }
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        game_sync_player_wait_turn(sync, i);
    }
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[i]));
    }

    free_sync(sync);
}

/*
 * Mixing writer_enter/exit cycles with interleaved reader_enter/exit
 * must always converge back to the fully unlocked baseline. We run a
 * couple of rounds to catch any off by one in the reader count.
 */
static void test_writer_reader_interleaved_cycles(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    for (uint32_t round = 0; round < 4; round++) {
        game_sync_writer_enter(sync);
        game_sync_writer_exit(sync);

        game_sync_reader_enter(sync);
        game_sync_reader_enter(sync);
        game_sync_reader_exit(sync);
        game_sync_reader_exit(sync);
    }

    CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->readers_count_mutex));

    free_sync(sync);
}

/*
 * game_sync_destroy must not crash on a freshly initialized sync. We
 * cannot really observe much more than that because sem_destroy leaves
 * the memory in an unspecified state, but running it through a sanitized
 * build would at least surface double free or use-after-free bugs.
 */
static void test_destroy_on_fresh_sync_does_not_crash(CuTest *tc) {
    game_sync_t *sync = calloc(1, sizeof(game_sync_t));
    CuAssertPtrNotNull(tc, sync);

    game_sync_init(sync);
    game_sync_destroy(sync);
    free(sync);

    /*
     * Reaching this assertion means destroy returned normally. We pass a
     * trivial true condition so CuTest still records the test as run.
     */
    CuAssertTrue(tc, 1);
}

CuSuite *game_sync_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_init_sets_expected_initial_values);
    SUITE_ADD_TEST(suite, test_writer_enter_exit_is_balanced);
    SUITE_ADD_TEST(suite, test_single_reader_enter_exit_is_balanced);
    SUITE_ADD_TEST(suite, test_multiple_readers_only_first_takes_mutex);
    SUITE_ADD_TEST(suite, test_view_may_render_post_wait_cycle);
    SUITE_ADD_TEST(suite, test_view_rendered_post_wait_cycle);
    SUITE_ADD_TEST(suite, test_player_turn_grant_is_isolated);
    SUITE_ADD_TEST(suite, test_player_turn_grant_all_players);
    SUITE_ADD_TEST(suite, test_writer_reader_interleaved_cycles);
    SUITE_ADD_TEST(suite, test_destroy_on_fresh_sync_does_not_crash);
    return suite;
}
