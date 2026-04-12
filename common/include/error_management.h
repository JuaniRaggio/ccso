#pragma once

#include <errno.h>
#include <stdint.h>

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
    mapping_error,
    connection_error,
    unreachable,
} error_code_t;

typedef struct {
    const char *file;
    const char *func;
    uint64_t line;
} trace_t;

#define HERE ((trace_t){__FILE__, __func__, __LINE__})
#define TRACE_NONE ((trace_t){NULL, NULL, 0})

typedef error_code_t (*error_manager_t)(trace_t internal, trace_t caller, error_code_t code);

error_code_t manage_error(trace_t internal, trace_t caller, error_code_t code);
