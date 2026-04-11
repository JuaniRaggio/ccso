#pragma once

#include <stdint.h>
#include <sys/errno.h>
#include <sys/mman.h>

typedef enum {
    access_error = EACCES,
    range_error = ERANGE,
    file_exists_error = EEXIST,
    invalid_argument_error = EINVAL,
    mapping_error,
    connection_error,
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
