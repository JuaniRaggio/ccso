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

#include <game_sync.h>

#include <game_sync.h>

typedef struct {
   const char *sharedMemoryName;
   size_t totalSize;
   int openFlags;
   int permissions;
   int protections;
   int mapFlag;
   off_t offset;
} shm_data_t;

/*
 * @brief creates and maps shared memory to it's return value
 *        uses errno for error management and returns NULL
 */

void *createSharedMemory(shm_data_t *data, error_manager_t manage_error, const char *file, const char *func,
                         uint64_t line);

void initalizeGameSync(game_sync_t *sharedGameSync);
