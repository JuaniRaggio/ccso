#pragma once

#include <errno.h>
#include <stddef.h>
#include <sys/_types/_off_t.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>

typedef void (*error_manager_t)();

/*
 * @brief creates and maps shared memory to it's return value
 *        uses errno for error management and returns NULL
 */
void *createSharedMemory(char *sharedMemoryName, size_t totalSize, int openFlags, int permissions, int proteccions,
                         int mapFlag, off_t offset, error_manager_t manage_errno);
