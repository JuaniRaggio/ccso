#include "error_management.h"
#include <shmemory_utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <semaphore.h>

static inline bool is_creator(int open_flags) {
    return open_flags & O_CREAT;
}

void *_create_shm(const shm_data_t *data, size_t shm_size, error_manager_t manage_error, const char *file,
                  const char *func, uint64_t line) {
    int fd = shm_open(data->shared_memory_name, data->open_flags, data->permissions);
    if (fd == -1) {
        manage_error(file, func, line, errno);
        return NULL;
    }

    // O_CREAT means that you are the creator of a new shared memory object
    if (is_creator(data->open_flags) && ftruncate(fd, shm_size) == -1) {
        manage_error(file, func, line, errno);
        close(fd);
        return NULL;
    }

    void *ptr = mmap(NULL, shm_size, data->protections, data->map_flag, fd, data->offset);
    if (ptr == MAP_FAILED) {
        manage_error(file, func, line, mapping_error);
        close(fd);
        return NULL;
    }
    close(fd);
    return ptr;
}

void _destroy_shm(void *ptr, size_t shm_size, error_manager_t manage_error, const char *file, const char *func,
                  uint64_t line) {
    if (ptr == NULL) {
        manage_error(file, func, line, invalid_argument_error);
        return;
    }
    if (munmap(ptr, shm_size) == -1) {
        manage_error(file, func, line, errno);
    }
}
