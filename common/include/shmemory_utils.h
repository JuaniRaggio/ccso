#pragma once

#include <errno.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>

/*
 * @brief creates and maps shared memory to it's return value
 *        uses errno for error management and returns NULL
 */
void *createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions,
                         int mapFlag);
