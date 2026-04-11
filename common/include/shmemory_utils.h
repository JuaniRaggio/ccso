#pragma once

#include <error_management.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <game_state.h>
#include <game_sync.h>

typedef struct {
    const char *shared_memory_name;
    int open_flags;
    int permissions;
    int protections;
    int map_flag;
    off_t offset;
} shm_data_t;

/*
 * @brief creates and maps shared memory to it's return value
 *        uses errno for error management and returns NULL
 *
 * _create_shm / _destroy_shm are the implementation. Callers that already
 * received file/func/line as parameters (trace forwarding) should call them
 * directly. Top-level callers should use the create_shm / destroy_shm macros
 * so that __FILE__ / __func__ / __LINE__ are captured at the call site.
 */
void *_create_shm(const shm_data_t *data, size_t shm_size, error_manager_t manage_error, trace_t caller);

void _destroy_shm(void *ptr, size_t shm_size, error_manager_t manage_error, trace_t caller);

#define create_shm(data, shm_size) _create_shm((data), (shm_size), manage_error, HERE)

#define destroy_shm(ptr, shm_size) _destroy_shm((ptr), (shm_size), manage_error, HERE)
