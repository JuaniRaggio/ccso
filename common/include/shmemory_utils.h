/**
 * @file shmemory_utils.h
 * @brief POSIX shared memory creation, mapping, and teardown utilities.
 *
 * Wraps shm_open / ftruncate / mmap into a single call, parameterised by
 * a @ref shm_data_t descriptor. Errors are forwarded through the
 * @ref error_manager_t callback chain with two-level source traces.
 *
 * Top-level callers should use the convenience macros @ref create_shm and
 * @ref destroy_shm, which automatically capture __FILE__ / __func__ / __LINE__.
 */
#pragma once

#include <error_management.h>
#include <errno.h>
#include <fcntl.h>
#include <game_state.h>
#include <game_sync.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

/**
 * @brief Configuration for a POSIX shared memory segment.
 *
 * Groups all parameters needed by shm_open + mmap into a single descriptor
 * so that entity-specific tables can be defined at compile time.
 */
typedef struct {
    const char *shared_memory_name;
    int open_flags;
    int permissions;
    int protections;
    int map_flag;
    off_t offset;
} shm_data_t;

/**
 * @brief Create (or open) and memory-map a shared memory segment.
 *
 * If O_CREAT is present in @p data->open_flags the segment is created and
 * truncated to @p shm_size bytes. Otherwise an existing segment is opened.
 *
 * @param data          Shared memory configuration descriptor.
 * @param shm_size      Required size in bytes.
 * @param manage_error  Error reporting callback.
 * @param caller        Caller source location for trace forwarding.
 * @return              Pointer to the mapped region, or NULL on failure.
 */
void *_create_shm(const shm_data_t *data, size_t shm_size, error_manager_t manage_error, trace_t caller);

/**
 * @brief Unmap a previously mapped shared memory region.
 *
 * @param ptr           Pointer returned by @ref _create_shm.
 * @param shm_size      Size that was passed to _create_shm.
 * @param manage_error  Error reporting callback.
 * @param caller        Caller source location for trace forwarding.
 */
void _destroy_shm(void *ptr, size_t shm_size, error_manager_t manage_error, trace_t caller);

/** @brief Convenience macro that captures the call-site trace automatically. */
#define create_shm(data, shm_size) _create_shm((data), (shm_size), manage_error, HERE)

/** @brief Convenience macro that captures the call-site trace automatically. */
#define destroy_shm(ptr, shm_size) _destroy_shm((ptr), (shm_size), manage_error, HERE)
