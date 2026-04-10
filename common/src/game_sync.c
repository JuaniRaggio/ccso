#include <game_sync.h>
#include <semaphore.h>

void game_sync_writer_enter(game_sync_t *sync) {
    sem_wait(&sync->master_writing);
    sem_wait(&sync->gamestate_mutex);
}

void game_sync_writer_exit(game_sync_t *sync) {
    sem_post(&sync->master_writing);
    sem_post(&sync->gamestate_mutex);
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
    sem_post(&sync->pending_changes_to_show);
}

void game_sync_wait_view_done(game_sync_t *sync) {
    sem_wait(&sync->printing);
}

void game_sync_view_wait_frame(game_sync_t *sync) {
    sem_wait(&sync->pending_changes_to_show);
}

void game_sync_view_frame_done(game_sync_t *sync) {
    sem_post(&sync->printing);
}

void game_sync_player_wait_turn(game_sync_t *sync, uint8_t player_idx) {
    sem_wait(&sync->player_may_send_movement[player_idx]);
}

void game_sync_player_grant_turn(game_sync_t *sync, uint8_t player_idx) {

    sem_post(&sync->player_may_send_movement[player_idx]);
}
