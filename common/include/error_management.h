#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
} errno_manager_t;

void manage_errno(const char *file, const char *func, uint64_t line);

void clear_error();
