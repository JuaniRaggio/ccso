#pragma once

#include <stdint.h>
#include <sys/errno.h>
#include <sys/mman.h>

typedef enum {
    access_error = EACCES,
    range_error = ERANGE,
    file_exists_error = EEXIST,
    entity_error,
    mapping_error,
} error_code_t;

typedef void (*error_manager_t)(const char *file, const char *func, uint64_t line, error_code_t code);

void manage_error(const char *file, const char *func, uint64_t line, error_code_t code);
