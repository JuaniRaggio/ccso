#pragma once

#include "view.h"
#include <game_state.h>
#include <stdint.h>

const char *display_name(const char *name);
void sort_players_by_score(game_state_t *state, int8_t order[]);

void view_draw_board(view_t *view, game_state_t *state);
void view_draw_panels(view_t *view, game_state_t *state);
void view_draw_endscreen(view_t *view, game_state_t *state);
