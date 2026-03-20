#include <shmemory_utils.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/fcntl.h>

inline bool is_creator(int openFlags) {
    return openFlags & O_CREAT;
}

void * createSharedMemory(char *sharedMemoryName, int totalSize, int openFlags, int permissions, int proteccions,
                         int mapFlag, int offset) {
   errno = 0;

   int fd = shm_open(sharedMemoryName, openFlags, permissions);
   if (fd == -1) {
      return NULL;
   }

   // O_CREAT means that you are the creator of a new shared memory object
   if (is_creator(openFlags) && ftruncate(fd, totalSize) == -1) {
         close(fd);
         return NULL;
   }

   void *ptr = mmap(NULL, totalSize, proteccions, mapFlag, fd, offset);
   if (ptr == MAP_FAILED) {
      close(fd);
      return NULL;
   }

   close(fd);
   return ptr;
}

