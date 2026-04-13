/**
 * @file game_admin.h
 * @brief Game administration: board init, player registration, move processing.
 *
 * Contains the core game logic executed by the master process: initialising
 * the board with random rewards, registering players, validating and applying
 * moves, processing full rounds, and printing final results.
 */
#pragma once

#include "game_state.h"
#include "pipes.h"
#include "player_protocol.h"
#include <game.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/types.h>

/**
 * @brief Requirements for registering a player in the game.
 */
typedef struct {
    pid_t player_pid;
    const char name[MAX_NAME_LENGTH];
} player_registration_requirements_t;

/**
 * @brief Summary of a single round of moves.
 */
typedef struct {
    bool any_move;
    bool any_valid;
    int8_t next_start_player;
} round_result_t;

/**
 * @brief Initialise the game board with random rewards.
 *
 * Seeds the PRNG with @p seed, then fills every cell with a random
 * value in [1, 9]. Also sets the board dimensions and player count.
 *
 * @param game    Game handle whose state will be initialised.
 * @param width   Board width in cells.
 * @param height  Board height in cells.
 * @param seed    PRNG seed.
 * @param players Number of players to register.
 */
void game_state_init(game_t *game, uint16_t width, uint16_t height, uint64_t seed, int8_t players);

/**
 * @brief Register a single player at a given index.
 *
 * @param current_players Player array in the game state.
 * @param idx             Index at which to register.
 * @param to_register     Registration data (PID and name).
 * @return                true on success, false if @p idx is out of range
 *                        or @p name is empty.
 */
bool game_register_player(player_t current_players[MAX_PLAYERS], size_t idx,
                          player_registration_requirements_t to_register);

/**
 * @brief Register up to MAX_PLAYERS players from an array of requirements.
 *
 * @param current_players Player array in the game state.
 * @param to_register     Array of registration requirements.
 * @return                Number of players successfully registered.
 */
size_t game_register_all(player_t current_players[MAX_PLAYERS],
                         player_registration_requirements_t to_register[MAX_PLAYERS]);

/**
 * @brief Check whether a board cell can be moved to.
 *
 * A move is allowed if the target coordinates are within bounds and the
 * cell value is positive (uncollected reward).
 *
 * @param state            Game state with the board.
 * @param vertical_coord   Target row.
 * @param horizontal_coord Target column.
 * @return                 true if the move is valid, false otherwise.
 */
bool is_move_allowed(game_state_t *state, uint16_t vertical_coord, uint16_t horizontal_coord);

/**
 * @brief Apply a player's move to the board.
 *
 * If the target cell is valid, the player collects its reward, and the
 * cell is marked with the player's index (negated). If invalid, only the
 * invalid-move counter is incremented.
 *
 * @param state            Game state.
 * @param vertical_coord   Target row.
 * @param horizontal_coord Target column.
 * @param playerId         Zero-based player index.
 */
void apply_move(game_state_t *state, uint16_t vertical_coord, uint16_t horizontal_coord, int8_t playerId);

/**
 * @brief Increment the valid or invalid move counter for a player.
 *
 * @param state         Game state.
 * @param is_valid_move true for a valid move, false for an invalid one.
 * @param playerId      Zero-based player index.
 */
void register_move(game_state_t *state, bool is_valid_move, int8_t playerId);

/**
 * @brief Validate and apply a directional move for a player.
 *
 * Converts the wire direction to board coordinates and delegates to
 * @ref apply_move.
 *
 * @param state      Game state.
 * @param player_idx Zero-based player index.
 * @param direction  Wire-encoded direction from the player.
 * @return           true if the resulting move was valid, false otherwise.
 */
bool process_player_move(game_state_t *state, uint8_t player_idx, direction_wire_t direction);

/**
 * @brief Handle a single player's turn in a select-driven round.
 *
 * Reads a direction from the player's pipe (if ready), acquires the
 * writer lock, processes the move, releases the lock, and grants the
 * player its next turn token.
 *
 * @param game       Game handle.
 * @param pipes      Pipe array.
 * @param readFds    fd_set returned by select() for this round.
 * @param masterSet  Persistent fd_set (modified if the player disconnects).
 * @param idx        Zero-based player index.
 * @param[out] out_valid  Set to true if the move was valid.
 * @return           true if the player was processed, false if skipped
 *                   (dead or not ready).
 */
bool handle_player_turn(game_t *game, int32_t pipes[][pipe_ends], fd_set *readFds, fd_set *masterSet, int8_t idx,
                        bool *out_valid);

/**
 * @brief Register all players using the basenames of their binary paths.
 *
 * Extracts the filename component from each path and stores it as the
 * player's display name.
 *
 * @param state Game state.
 * @param paths Array of player binary paths (one per player).
 */
void register_players_from_paths(game_state_t *state, const char *paths[]);

/**
 * @brief Check whether at least one player is still alive.
 *
 * @param state Game state.
 * @return      true if any player has @c state == true, false otherwise.
 */
bool any_player_alive(game_state_t *state);

/**
 * @brief Place all players on the board in a grid pattern.
 *
 * Distributes players evenly across the board surface and marks their
 * starting cells with the negated player index.
 *
 * @param state Game state (must have players_count and board dimensions set).
 */
void place_players_on_board(game_state_t *state);

/**
 * @brief Process one full round: read moves from all ready players.
 *
 * Iterates over all players starting from @p start_player (round-robin),
 * calling @ref handle_player_turn for each.
 *
 * @param game         Game handle.
 * @param pipes        Pipe array.
 * @param readFds      fd_set returned by select().
 * @param masterSet    Persistent fd_set.
 * @param start_player Index of the first player to process this round.
 * @return             Round summary with flags and next start index.
 */
round_result_t process_round(game_t *game, int32_t pipes[][pipe_ends], fd_set *readFds, fd_set *masterSet,
                             int8_t start_player);

/**
 * @brief Print final scores for all players to stderr.
 *
 * Output format per player:
 * @code
 * Player <name> (<idx>): score=<N> valid=<N> invalid=<N>
 * @endcode
 *
 * @param state Game state with final player statistics.
 */
void print_game_results(game_state_t *state);
