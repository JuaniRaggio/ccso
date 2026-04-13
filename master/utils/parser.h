/**
 * @file parser.h
 * @brief Master process command-line argument parser.
 *
 * Parses the master's flags (-w, -h, -d, -t, -s, -v, -p) using getopt
 * and populates a @ref parameters_t structure. Errors are reported via
 * a bitmask of @ref parameter_status_t values.
 */
#pragma once

#include <stdint.h>
#include <game_state.h>

/**
 * @brief Parsed command-line parameters for the master process.
 */
typedef struct {
    uint64_t players_count;
    uint64_t seed;
    uint64_t width;
    char *c_width;
    uint64_t height;
    char *c_height;
    uint64_t delay;
    uint64_t timeout;
    const char *view_path;
    const char *players_paths[MAX_PLAYERS];
} parameters_t;

/**
 * @brief Bitmask flags indicating parsing errors.
 *
 * Multiple flags can be OR'd together when more than one error is detected.
 */
typedef enum {
    success = 0x0,
    invalid_integer_type = 0x1,
    exceeded_player_limit = invalid_integer_type << 1,
    unknown_optional_flag = exceeded_player_limit << 1,
    overflow = unknown_optional_flag << 1,
} parameter_status_t;

/**
 * @brief Parse the master's command-line arguments.
 *
 * Uses getopt internally; argument strings are borrowed (not copied).
 *
 * @param argc       Argument count from main().
 * @param argv       Argument vector from main().
 * @param[out] parameters  Receives the parsed values. Fields not covered
 *                         by the provided flags retain their initial values.
 * @return @ref success on success, or a bitmask of error flags.
 */
parameter_status_t parse(int argc, char *argv[], parameters_t *parameters);
