/**
 * @file signals.h
 * @brief Process-wide signal handling for graceful shutdown.
 *
 * Installs handlers for SIGINT and SIGTERM that set an internal flag
 * queryable via @ref was_interrupted. Used by master, player, and view
 * processes to cooperatively exit their main loops.
 */
#pragma once

#include <stdbool.h>

/**
 * @brief Install signal handlers for SIGINT and SIGTERM.
 *
 * Must be called once at the start of each process. Subsequent signals
 * set an internal flag rather than terminating the process.
 */
void setup_signals(void);

/**
 * @brief Check whether a termination signal has been received.
 *
 * @return true if SIGINT or SIGTERM was caught since @ref setup_signals
 *         was called, false otherwise.
 */
bool was_interrupted(void);
