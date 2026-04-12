#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    CuAssertIntEquals_Msg(tc, "mutex released only by last reader", (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));

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

/*
 * Re-initializing an already-initialized game_sync_t must leave the struct
 * in the same baseline state as a fresh init: view gates locked, writer
 * and mutexes unlocked, reader count at zero. We intentionally dirty a
 * few counters first so we can prove game_sync_init really resets them.
 *
 * Note: we need to destroy the semaphores from the first init before
 * re-initing, otherwise valgrind complains about the leaked sem resources
 * and the second sem_init would trample over a live semaphore.
 */
static void test_reinit_after_destroy_restores_baseline(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    /* Dirty the counters so we can prove the re-init wipes them. */
    game_sync_reader_enter(sync);
    game_sync_reader_enter(sync);
    game_sync_notify_view(sync);
    game_sync_view_frame_done(sync);
    game_sync_player_grant_turn(sync, 0);
    CuAssertIntEquals(tc, 2, (int)sync->readers_count);

    /* Destroy the currently-held semaphores, then re-init in place. */
    game_sync_destroy(sync);
    game_sync_init(sync);

    CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_may_render));
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_rendered));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[i]));
    }

    free_sync(sync);
}

/*
 * A stress run of many short writer critical sections should converge
 * back to the baseline every single iteration. This catches off by one
 * or inverted post order in writer_exit.
 */
static void test_writer_stress_many_iterations(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    static const uint32_t iterations = 1000;
    for (uint32_t i = 0; i < iterations; i++) {
        game_sync_writer_enter(sync);
        CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));
        game_sync_writer_exit(sync);
    }
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));

    free_sync(sync);
}

/*
 * Same idea with readers: enter/exit many times and verify the reader
 * count never drifts. The gamestate mutex must also end up unlocked at
 * the end of the cycle.
 */
static void test_reader_stress_many_iterations(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    static const uint32_t iterations = 1000;
    for (uint32_t i = 0; i < iterations; i++) {
        game_sync_reader_enter(sync);
        CuAssertIntEquals(tc, 1, (int)sync->readers_count);
        game_sync_reader_exit(sync);
        CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    }
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));

    free_sync(sync);
}

/*
 * Reader slot 0 and the highest slot (MAX_PLAYERS - 1) are the two edges
 * of the per-player gate array. Make sure grant/wait works on both.
 */
static void test_player_turn_boundary_slots(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    game_sync_player_grant_turn(sync, 0);
    CuAssertIntEquals(tc, 1, sem_value(&sync->player_may_send_movement[0]));
    game_sync_player_wait_turn(sync, 0);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[0]));

    game_sync_player_grant_turn(sync, MAX_PLAYERS - 1);
    CuAssertIntEquals(tc, 1, sem_value(&sync->player_may_send_movement[MAX_PLAYERS - 1]));
    game_sync_player_wait_turn(sync, MAX_PLAYERS - 1);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[MAX_PLAYERS - 1]));

    /* All other slots must remain untouched. */
    for (uint8_t i = 1; i < MAX_PLAYERS - 1; i++) {
        CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[i]));
    }

    free_sync(sync);
}

/*
 * Multiple posts on the same view_may_render gate without intermediate
 * waits should accumulate the semaphore value. The view can then drain
 * them one by one.
 */
static void test_view_may_render_accumulates(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    static const int32_t frames = 5;
    for (int32_t i = 0; i < frames; i++) {
        game_sync_notify_view(sync);
    }
    CuAssertIntEquals(tc, frames, sem_value(&sync->view_may_render));

    for (int32_t i = 0; i < frames; i++) {
        game_sync_view_wait_frame(sync);
    }
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_may_render));

    free_sync(sync);
}

/*
 * Concurrency: N reader threads acquire reader_enter in parallel. While
 * they are inside the critical section the gamestate mutex must be held
 * (value 0) and the readers_count must equal N. After they all exit it
 * must drop back to zero and the mutex must be unlocked.
 *
 * We use a pthread barrier so all readers block together and the main
 * thread can observe the in-flight state deterministically.
 */
typedef struct {
    game_sync_t *sync;
    pthread_barrier_t *enter_barrier;  // synchronize the enter
    pthread_barrier_t *inside_barrier; // main thread waits here
    pthread_barrier_t *exit_barrier;   // main thread releases readers
} reader_thread_ctx_t;

static void *reader_worker(void *arg) {
    reader_thread_ctx_t *ctx = (reader_thread_ctx_t *)arg;

    pthread_barrier_wait(ctx->enter_barrier);
    game_sync_reader_enter(ctx->sync);

    /* Signal main thread we're inside, wait for permission to leave. */
    pthread_barrier_wait(ctx->inside_barrier);
    pthread_barrier_wait(ctx->exit_barrier);

    game_sync_reader_exit(ctx->sync);
    return NULL;
}

static void test_concurrent_readers(CuTest *tc) {
    static const int32_t num_readers = 4;

    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    pthread_barrier_t enter_barrier;
    pthread_barrier_t inside_barrier;
    pthread_barrier_t exit_barrier;
    CuAssertIntEquals(tc, 0, pthread_barrier_init(&enter_barrier, NULL, num_readers));
    CuAssertIntEquals(tc, 0, pthread_barrier_init(&inside_barrier, NULL, num_readers + 1));
    CuAssertIntEquals(tc, 0, pthread_barrier_init(&exit_barrier, NULL, num_readers + 1));

    reader_thread_ctx_t ctx = {
        .sync = sync,
        .enter_barrier = &enter_barrier,
        .inside_barrier = &inside_barrier,
        .exit_barrier = &exit_barrier,
    };

    pthread_t threads[num_readers];
    for (int32_t i = 0; i < num_readers; i++) {
        CuAssertIntEquals(tc, 0, pthread_create(&threads[i], NULL, reader_worker, &ctx));
    }

    /* Wait for every reader to finish its enter. */
    pthread_barrier_wait(&inside_barrier);

    CuAssertIntEquals(tc, num_readers, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->gamestate_mutex));

    /* Let them all exit. */
    pthread_barrier_wait(&exit_barrier);
    for (int32_t i = 0; i < num_readers; i++) {
        CuAssertIntEquals(tc, 0, pthread_join(threads[i], NULL));
    }

    CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));

    CuAssertIntEquals(tc, 0, pthread_barrier_destroy(&enter_barrier));
    CuAssertIntEquals(tc, 0, pthread_barrier_destroy(&inside_barrier));
    CuAssertIntEquals(tc, 0, pthread_barrier_destroy(&exit_barrier));

    free_sync(sync);
}

/*
 * Concurrency: a writer thread should be blocked while readers hold the
 * lock. We start a reader in the main thread, spawn a writer thread, and
 * verify via a timed sem_trywait on a private "done" semaphore that the
 * writer has not completed its critical section until we release the
 * reader.
 */
typedef struct {
    game_sync_t *sync;
    sem_t *done;
} writer_thread_ctx_t;

static void *writer_worker(void *arg) {
    writer_thread_ctx_t *ctx = (writer_thread_ctx_t *)arg;
    game_sync_writer_enter(ctx->sync);
    game_sync_writer_exit(ctx->sync);
    sem_post(ctx->done);
    return NULL;
}

static void test_writer_blocks_while_reader_active(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    sem_t done;
    CuAssertIntEquals(tc, 0, sem_init(&done, 0, 0));

    /* Main thread holds a reader lock. */
    game_sync_reader_enter(sync);

    writer_thread_ctx_t ctx = {.sync = sync, .done = &done};
    pthread_t writer_thread;
    CuAssertIntEquals(tc, 0, pthread_create(&writer_thread, NULL, writer_worker, &ctx));

    /* Give the writer time to try entering; it should still be blocked. */
    struct timespec wait = {.tv_sec = 0, .tv_nsec = 50 * 1000 * 1000}; // 50ms
    nanosleep(&wait, NULL);
    CuAssertIntEquals_Msg(tc, "writer must not progress while reader holds lock", -1, sem_trywait(&done));
    CuAssertIntEquals(tc, EAGAIN, errno);

    /* Release the reader; the writer must then finish. */
    game_sync_reader_exit(sync);

    /* Wait for the writer to finish (bounded timed wait to avoid hangs). */
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 5;
    CuAssertIntEquals(tc, 0, sem_timedwait(&done, &deadline));

    CuAssertIntEquals(tc, 0, pthread_join(writer_thread, NULL));
    CuAssertIntEquals(tc, 0, sem_destroy(&done));

    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, 0, (int)sync->readers_count);

    free_sync(sync);
}

/*
 * Concurrency: a reader that arrives while a writer is inside must wait
 * on the turnstile (sem_wait on master_writing). We assert that the
 * reader does not progress until the writer exits. This is the symmetric
 * test of test_writer_blocks_while_reader_active.
 */
typedef struct {
    game_sync_t *sync;
    sem_t *done;
} reader_wait_ctx_t;

static void *reader_wait_worker(void *arg) {
    reader_wait_ctx_t *ctx = (reader_wait_ctx_t *)arg;
    game_sync_reader_enter(ctx->sync);
    game_sync_reader_exit(ctx->sync);
    sem_post(ctx->done);
    return NULL;
}

static void test_reader_blocks_while_writer_active(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    sem_t done;
    CuAssertIntEquals(tc, 0, sem_init(&done, 0, 0));

    /* Main thread becomes the writer. */
    game_sync_writer_enter(sync);

    reader_wait_ctx_t ctx = {.sync = sync, .done = &done};
    pthread_t reader_thread;
    CuAssertIntEquals(tc, 0, pthread_create(&reader_thread, NULL, reader_wait_worker, &ctx));

    struct timespec wait = {.tv_sec = 0, .tv_nsec = 50 * 1000 * 1000}; // 50ms
    nanosleep(&wait, NULL);
    CuAssertIntEquals_Msg(tc, "reader must not progress while writer holds lock", -1, sem_trywait(&done));
    CuAssertIntEquals(tc, EAGAIN, errno);

    game_sync_writer_exit(sync);

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 5;
    CuAssertIntEquals(tc, 0, sem_timedwait(&done, &deadline));

    CuAssertIntEquals(tc, 0, pthread_join(reader_thread, NULL));
    CuAssertIntEquals(tc, 0, sem_destroy(&done));

    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, 0, (int)sync->readers_count);

    free_sync(sync);
}

/*
 * Concurrency: the master notifies the view and waits for it to finish a
 * frame. We simulate the view in a worker thread that calls
 * view_wait_frame, does "work", and then view_frame_done. The main thread
 * acts as the master: notify_view, wait_view_done.
 */
typedef struct {
    game_sync_t *sync;
} view_thread_ctx_t;

static void *view_worker(void *arg) {
    view_thread_ctx_t *ctx = (view_thread_ctx_t *)arg;
    game_sync_view_wait_frame(ctx->sync);
    /* Simulate rendering work. */
    game_sync_view_frame_done(ctx->sync);
    return NULL;
}

static void test_view_frame_handoff(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    view_thread_ctx_t ctx = {.sync = sync};
    pthread_t view_thread;
    CuAssertIntEquals(tc, 0, pthread_create(&view_thread, NULL, view_worker, &ctx));

    game_sync_notify_view(sync);
    game_sync_wait_view_done(sync);

    CuAssertIntEquals(tc, 0, pthread_join(view_thread, NULL));

    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_may_render));
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_rendered));

    free_sync(sync);
}

/*
 * Multiple posts on view_rendered without intermediate waits should
 * accumulate, symmetrically to view_may_render.
 */
static void test_view_rendered_accumulates(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    static const int32_t frames = 3;
    for (int32_t i = 0; i < frames; i++) {
        game_sync_view_frame_done(sync);
    }
    CuAssertIntEquals(tc, frames, sem_value(&sync->view_rendered));

    for (int32_t i = 0; i < frames; i++) {
        game_sync_wait_view_done(sync);
    }
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_rendered));

    free_sync(sync);
}

/*
 * Multiple grants on the same player slot without intermediate waits
 * should accumulate the semaphore value.
 */
static void test_player_turn_grant_accumulates(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    const uint8_t slot = 2;
    game_sync_player_grant_turn(sync, slot);
    game_sync_player_grant_turn(sync, slot);
    game_sync_player_grant_turn(sync, slot);

    CuAssertIntEquals(tc, 3, sem_value(&sync->player_may_send_movement[slot]));

    game_sync_player_wait_turn(sync, slot);
    game_sync_player_wait_turn(sync, slot);
    game_sync_player_wait_turn(sync, slot);

    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[slot]));

    free_sync(sync);
}

/*
 * Stress: grant/wait for all players across many rounds. Each round
 * gives every player a turn and immediately consumes it.
 */
static void test_player_turn_stress_all_players(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    static const uint32_t rounds = 200;
    for (uint32_t r = 0; r < rounds; r++) {
        for (uint8_t p = 0; p < MAX_PLAYERS; p++) {
            game_sync_player_grant_turn(sync, p);
            game_sync_player_wait_turn(sync, p);
        }
    }

    for (uint8_t p = 0; p < MAX_PLAYERS; p++) {
        CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[p]));
    }

    free_sync(sync);
}

/*
 * Stress: the full master loop: writer critical section, then multiple
 * readers, across many rounds. This simulates the game main loop pattern
 * with a single thread.
 */
static void test_full_loop_stress(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    static const uint32_t rounds = 500;
    for (uint32_t r = 0; r < rounds; r++) {
        /* Master writes. */
        game_sync_writer_enter(sync);
        game_sync_writer_exit(sync);

        /* Several readers read. */
        game_sync_reader_enter(sync);
        game_sync_reader_enter(sync);
        game_sync_reader_enter(sync);
        game_sync_reader_exit(sync);
        game_sync_reader_exit(sync);
        game_sync_reader_exit(sync);

        /* Notify and ack view. */
        game_sync_notify_view(sync);
        game_sync_view_wait_frame(sync);
        game_sync_view_frame_done(sync);
        game_sync_wait_view_done(sync);
    }

    CuAssertIntEquals(tc, 0, (int)sync->readers_count);
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->master_writing));
    CuAssertIntEquals(tc, (int)SEM_UNLOCKED, sem_value(&sync->gamestate_mutex));
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_may_render));
    CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->view_rendered));

    free_sync(sync);
}

/*
 * Concurrency: the master grants turns to all players in parallel.
 * Each player is simulated by a worker thread that waits for its turn
 * and then signals completion. The main thread grants all turns, then
 * joins all threads.
 */
typedef struct {
    game_sync_t *sync;
    uint8_t player_idx;
    sem_t *done;
} player_thread_ctx_t;

static void *player_turn_worker(void *arg) {
    player_thread_ctx_t *ctx = (player_thread_ctx_t *)arg;
    game_sync_player_wait_turn(ctx->sync, ctx->player_idx);
    sem_post(ctx->done);
    return NULL;
}

static void test_concurrent_player_turns(CuTest *tc) {
    game_sync_t *sync = make_sync();
    CuAssertPtrNotNull(tc, sync);

    sem_t done[MAX_PLAYERS];
    player_thread_ctx_t ctxs[MAX_PLAYERS];
    pthread_t threads[MAX_PLAYERS];

    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, 0, sem_init(&done[i], 0, 0));
        ctxs[i] = (player_thread_ctx_t){.sync = sync, .player_idx = i, .done = &done[i]};
        CuAssertIntEquals(tc, 0, pthread_create(&threads[i], NULL, player_turn_worker, &ctxs[i]));
    }

    /* Small delay to let all player threads block on their semaphores. */
    struct timespec wait = {.tv_sec = 0, .tv_nsec = 30 * 1000 * 1000};
    nanosleep(&wait, NULL);

    /* Grant all turns simultaneously. */
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        game_sync_player_grant_turn(sync, i);
    }

    /* Wait for all players to finish. */
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 5;
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, 0, sem_timedwait(&done[i], &deadline));
    }

    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        CuAssertIntEquals(tc, 0, pthread_join(threads[i], NULL));
        CuAssertIntEquals(tc, 0, sem_destroy(&done[i]));
        CuAssertIntEquals(tc, (int)SEM_LOCKED, sem_value(&sync->player_may_send_movement[i]));
    }

    free_sync(sync);
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
    SUITE_ADD_TEST(suite, test_reinit_after_destroy_restores_baseline);
    SUITE_ADD_TEST(suite, test_writer_stress_many_iterations);
    SUITE_ADD_TEST(suite, test_reader_stress_many_iterations);
    SUITE_ADD_TEST(suite, test_player_turn_boundary_slots);
    SUITE_ADD_TEST(suite, test_view_may_render_accumulates);
    SUITE_ADD_TEST(suite, test_concurrent_readers);
    SUITE_ADD_TEST(suite, test_writer_blocks_while_reader_active);
    SUITE_ADD_TEST(suite, test_reader_blocks_while_writer_active);
    SUITE_ADD_TEST(suite, test_view_frame_handoff);
    SUITE_ADD_TEST(suite, test_view_rendered_accumulates);
    SUITE_ADD_TEST(suite, test_player_turn_grant_accumulates);
    SUITE_ADD_TEST(suite, test_player_turn_stress_all_players);
    SUITE_ADD_TEST(suite, test_full_loop_stress);
    SUITE_ADD_TEST(suite, test_concurrent_player_turns);
    return suite;
}
