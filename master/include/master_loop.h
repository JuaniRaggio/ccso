#pragma once

#include "pipes.h"
#include <game.h>
#include <parser.h>
#include <stdbool.h>
#include <sys/types.h>

bool master_run(game_t *game, parameters_t *params, int32_t pipes[][pipe_ends],
                pid_t view_pid, bool *has_view);
