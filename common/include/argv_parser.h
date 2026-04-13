/**
 * @file argv_parser.h
 * @brief Command-line argument parser for child processes (player, view).
 *
 * Player and view binaries receive the board dimensions as positional
 * arguments from the master process. This module extracts and validates
 * the width and height values from argv.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Parse board dimensions from the process argument vector.
 *
 * Expects argv[1] and argv[2] to be decimal strings representing
 * the board width and height respectively.
 *
 * @param[in]  argv       Argument vector passed to main().
 * @param[out] out_width  Parsed board width.
 * @param[out] out_height Parsed board height.
 * @return true on success, false if either value is not a valid integer.
 */
bool parse_board_args(char *argv[], uint16_t *out_width, uint16_t *out_height);
