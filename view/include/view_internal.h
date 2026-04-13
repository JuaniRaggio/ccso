/**
 * @file view_internal.h
 * @brief Internal rendering helpers for the view process.
 *
 * These functions are used by view.c to draw individual UI components.
 * Not intended for direct use outside the view module.
 */
#pragma once

#include "view.h"
#include <game_state.h>
#include <stdint.h>

/**
 * @brief Strip the "player-" prefix from a player name for display.
 *
 * If @p name starts with "player-", returns a pointer past the prefix.
 * Otherwise returns @p name unchanged.
 *
 * @param name Raw player name.
 * @return     Display-friendly name (pointer into @p name, not a copy).
 */
const char *display_name(const char *name);

/**
 * @brief Sort player indices by score in descending order.
 *
 * Populates @p order with player indices [0..players_count) sorted so
 * that order[0] is the player with the highest score.
 *
 * @param state     Game state with player scores.
 * @param[out] order  Array of at least @c players_count elements to fill.
 */
void sort_players_by_score(game_state_t *state, int8_t order[]);

/**
 * @brief Draw the game board in the board window.
 *
 * Renders each cell with its reward value or the player's theme emoji.
 * Colour coding distinguishes players, empty cells, and uncollected rewards.
 *
 * @param view  View state with the board window and layout offsets.
 * @param state Game state with the board and player positions.
 */
void view_draw_board(view_t *view, game_state_t *state);

/**
 * @brief Draw the player status panels below the board.
 *
 * Shows each player's name, flag, score, and alive/dead status in a
 * horizontal row of bordered panels.
 *
 * @param view  View state with the panel window.
 * @param state Game state with player statistics.
 */
void view_draw_panels(view_t *view, game_state_t *state);

/**
 * @brief Draw the end-of-game leaderboard overlay.
 *
 * Centers a bordered window over the board showing the final ranking,
 * scores, and a "press any key" prompt.
 *
 * @param view  View state.
 * @param state Game state with final player statistics.
 */
void view_draw_endscreen(view_t *view, game_state_t *state);
