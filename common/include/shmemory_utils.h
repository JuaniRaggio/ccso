#pragma once

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <game_state.h>
#include <game_sync.h>

typedef void (*error_manager_t)();

/*
 * @brief creates and maps shared memory to it's return value
 *        uses errno for error management and returns NULL
 */

void *createSharedMemory(char *sharedMemoryName, size_t totalSize, int openFlags, int permissions, int proteccions,
                         int mapFlag, off_t offset /* error_manager_t manage_errno*/);

void initalizeGameSync(game_sync_t *sharedGameSync);
