#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/select.h>
#include "game_state.h"

typedef enum {
    invalid_pipe = -1,
    pipe_reader = 0,
    pipe_writer,
    pipe_ends,
} pipe_users_t;

void create_pipes(int pipes[][pipe_ends], int playersCount);

pid_t fork_view(const char *view_path, const char *width, const char *height);

void fork_players(int pipes[][pipe_ends], int playersCount, game_state_t *game_state, const char *player_paths[],
                  const char *width, const char *height);

void close_other_pipes(int32_t pipes[][pipe_ends], uint32_t pipe_count, ssize_t dont_close_this_pipe,
                       const pipe_users_t user_to_close);

void init_fd_set(fd_set *masterSet, int pipes[][pipe_ends], int playersCount, int *maxFd);

void disconnect_player(player_t *player, int32_t pipes[][pipe_ends], fd_set *master_set, int8_t idx);

void close_active_pipes(int32_t pipes[][pipe_ends], player_t players[], int8_t count);
