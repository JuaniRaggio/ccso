#include <game_sync.h>
#include <semaphore.h>
#include <sys/semaphore.h>

void game_sync_writer_enter(game_sync_t *sync) {
    sem_wait(&sync->master_writing);
    sem_wait(&sync->gamestate_mutex);
}

// IMPORTANT: post in reverse order than adquisition
void game_sync_writer_exit(game_sync_t *sync) {
    sem_post(&sync->gamestate_mutex);
    sem_post(&sync->master_writing);
}

void game_sync_reader_enter(game_sync_t *sync) {
    // turnstyle
    sem_wait(&sync->master_writing);
    sem_post(&sync->master_writing);

    // only 1 process moding counter
    sem_wait(&sync->readers_count_mutex);
    if (++sync->readers_count == 1) {
        sem_wait(&sync->gamestate_mutex);
    }
    sem_post(&sync->readers_count_mutex);
}

void game_sync_reader_exit(game_sync_t *sync) {
    sem_wait(&sync->readers_count_mutex);
    if (--sync->readers_count == 0) {
        sem_post(&sync->gamestate_mutex);
    }
    sem_post(&sync->readers_count_mutex);
}

void game_sync_notify_view(game_sync_t *sync) {
    sem_post(&sync->view_may_render);
}

void game_sync_wait_view_done(game_sync_t *sync) {
    sem_wait(&sync->view_rendered);
}

void game_sync_view_wait_frame(game_sync_t *sync) {
    sem_wait(&sync->view_may_render);
}

void game_sync_view_frame_done(game_sync_t *sync) {
    sem_post(&sync->view_rendered);
}

void game_sync_player_wait_turn(game_sync_t *sync, uint8_t player_idx) {
    sem_wait(&sync->player_may_send_movement[player_idx]);
}

void game_sync_player_grant_turn(game_sync_t *sync, uint8_t player_idx) {
    sem_post(&sync->player_may_send_movement[player_idx]);
}

void game_sync_init(game_sync_t *sync) {
    sem_init(&sync->view_may_render, SEM_SHARED_BETWEEN_PROCESSES, SEM_LOCKED);
    sem_init(&sync->view_rendered, SEM_SHARED_BETWEEN_PROCESSES, SEM_LOCKED);
    sem_init(&sync->master_writing, SEM_SHARED_BETWEEN_PROCESSES, SEM_UNLOCKED);
    sem_init(&sync->gamestate_mutex, SEM_SHARED_BETWEEN_PROCESSES, SEM_UNLOCKED);
    sem_init(&sync->readers_count_mutex, SEM_SHARED_BETWEEN_PROCESSES, SEM_UNLOCKED);
    sync->readers_count = 0;
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        sem_init(&sync->player_may_send_movement[i], SEM_SHARED_BETWEEN_PROCESSES, SEM_UNLOCKED);
    }
}

void game_sync_destroy(game_sync_t *sync) {
    sem_destroy(&sync->view_may_render);
    sem_destroy(&sync->view_rendered);
    sem_destroy(&sync->master_writing);
    sem_destroy(&sync->gamestate_mutex);
    sem_destroy(&sync->readers_count_mutex);
    sync->readers_count = 0;
    for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
        sem_destroy(&sync->player_may_send_movement[i]);
    }
}
