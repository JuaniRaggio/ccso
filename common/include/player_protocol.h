/**
 * @file player_protocol.h
 * @brief Wire protocol and direction types for player-master communication.
 *
 * Defines the eight cardinal/intercardinal directions, their (dx, dy) deltas,
 * and the read/write primitives used to exchange a single-byte direction
 * over a pipe between a player process and the master.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @brief Enumeration of the eight possible movement directions.
 *
 * Values are ordered clockwise starting from up (north).
 * @c dir_count serves as a sentinel equal to the total number of directions.
 */
typedef enum {
    dir_up = 0,
    dir_up_right,
    dir_right,
    dir_down_right,
    dir_down,
    dir_down_left,
    dir_left,
    dir_up_left,
    dir_count,
} direction_t;

/** @brief On-the-wire representation of a direction (single signed byte). */
typedef int8_t direction_wire_t;

/** @brief Sentinel value indicating the player has no valid move available. */
#define NO_VALID_MOVE ((direction_wire_t) - 1)

/**
 * @brief Displacement vector for a single movement step.
 */
typedef struct {
    int8_t dx;
    int8_t dy;
} move_delta_t;

/** @brief Lookup table mapping each @ref direction_t to its displacement. */
extern const move_delta_t move_delta[dir_count];

/**
 * @brief Check whether a wire-encoded direction is in the valid range.
 *
 * @param d Direction byte received from a player.
 * @return  true if @p d is in [0, dir_count), false otherwise.
 */
static inline bool is_valid_direction(direction_wire_t d) {
    return d >= 0 && d < dir_count;
}

/**
 * @brief Compute the destination coordinates after applying a direction.
 *
 * @param x      Current column.
 * @param y      Current row.
 * @param dir    Direction to apply.
 * @param[out] out_x  Resulting column.
 * @param[out] out_y  Resulting row.
 */
static inline void apply_direction(int16_t x, int16_t y, direction_t dir, int16_t *out_x, int16_t *out_y) {
    *out_x = x + move_delta[dir].dx;
    *out_y = y + move_delta[dir].dy;
}

/**
 * @brief Write a direction byte to a file descriptor (pipe write end).
 *
 * @param fd  File descriptor open for writing.
 * @param dir Direction to send.
 * @return    Number of bytes written (1 on success), or -1 on error.
 */
ssize_t send_direction(int32_t fd, direction_wire_t dir);

/**
 * @brief Read a direction byte from a file descriptor (pipe read end).
 *
 * @param fd         File descriptor open for reading.
 * @param[out] out_dir  Receives the decoded direction on success.
 * @return           Number of bytes read (1 on success), 0 on EOF, or -1 on error.
 */
ssize_t recv_direction(int32_t fd, direction_wire_t *out_dir);
