/**
 * @file error_management.h
 * @brief Centralized error reporting with source-location tracing.
 *
 * Provides a unified error handling mechanism that maps POSIX errno values
 * and custom error codes to human-readable descriptions. Errors are reported
 * to stderr with optional two-level stack traces (internal + caller).
 */
#pragma once

#include <errno.h>
#include <stdint.h>

/**
 * @brief Error codes used throughout the project.
 *
 * Standard codes reuse POSIX errno values directly. Custom codes start at 200
 * to avoid collisions with system-defined values.
 */
typedef enum {
    access_error = EACCES,
    range_error = ERANGE,
    file_exists_error = EEXIST,
    invalid_argument_error = EINVAL,
    no_memory_error = ENOMEM,
    no_entry_error = ENOENT,
    bad_fd_error = EBADF,
    io_error = EIO,
    broken_pipe_error = EPIPE,
    interrupted_error = EINTR,
    too_many_files_error = EMFILE,
    file_table_full_error = ENFILE,
    no_space_error = ENOSPC,
    resource_busy_error = EAGAIN,
    permission_error = EPERM,
    exec_format_error = ENOEXEC,
    mapping_error = 200,
    connection_error,
    unreachable,
} error_code_t;

/**
 * @brief Source location descriptor for error traces.
 *
 * Captures the file name, function name, and line number at a given call site.
 * Use the @ref HERE macro to build one automatically.
 */
typedef struct {
    const char *file;
    const char *func;
    uint64_t line;
} trace_t;

/** @brief Build a trace_t for the current call site. */
#define HERE ((trace_t){__FILE__, __func__, __LINE__})

/** @brief Null trace used when no caller context is available. */
#define TRACE_NONE ((trace_t){NULL, NULL, 0})

/**
 * @brief Function pointer type for error managers.
 *
 * @param internal  Trace from inside the failing function.
 * @param caller    Trace from the call site that triggered the operation.
 * @param code      The error code to report.
 * @return          The same @p code, allowing chained returns.
 */
typedef error_code_t (*error_manager_t)(trace_t internal, trace_t caller, error_code_t code);

/**
 * @brief Default error manager: logs the error description and traces to stderr.
 *
 * @param internal  Trace from inside the failing function.
 * @param caller    Trace from the call site that triggered the operation.
 * @param code      The error code to report.
 * @return          The same @p code passed in.
 */
error_code_t manage_error(trace_t internal, trace_t caller, error_code_t code);
