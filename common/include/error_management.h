#pragma once

#include <stdint.h>
#include <sys/errno.h>
#include <sys/mman.h>

typedef void (*error_manager_t)(const char *file, const char *func, uint64_t line);

typedef enum {
    access_error = EACCES,
    range_error = ERANGE,
    file_exists_error = EEXIST,
    mapping_error,
} error_code_t;

void manage_errno(const char *file, const char *func, uint64_t line);

void clear_error();
