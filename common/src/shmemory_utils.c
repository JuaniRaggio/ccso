#include <shmemory_utils.h>
#include <stdbool.h>
#include <semaphore.h>

bool is_creator(int openFlags) {
   return openFlags & O_CREAT;
}

void *createSharedMemory(char *sharedMemoryName, size_t totalSize, int openFlags, int permissions, int proteccions,
                         int mapFlag, off_t offset /*, error_manager_t errno_manager*/) {
   errno = 0;

   int fd = shm_open(sharedMemoryName, openFlags, permissions);
   if (fd == -1) {
      // errno_manager();
      return NULL;
   }

   // O_CREAT means that you are the creator of a new shared memory object
   if (is_creator(openFlags) && ftruncate(fd, totalSize) == -1) {
      // errno_manager();
      close(fd);
      return NULL;
   }

   void *ptr = mmap(NULL, totalSize, proteccions, mapFlag, fd, offset);
   if (ptr == MAP_FAILED) {
      // errno_manager();
      close(fd);
      return NULL;
   }

   close(fd);
   return ptr;
}

void initalizeGameSync(game_sync_t *sharedGameSync) {

   /*
   ** The 2nd paramter = 1, means that the Semaphore is shared between processes. If it was 0, the semaphore would only
   ** works between threats from the same process
   ** The 3rd parameter (usually 0 or 1) is the initial value of the Semaphore
   */

   // Sems for synchronization: master <-> view
   sem_init(&sharedGameSync->A, 1, 0); // starts bloqued
   sem_init(&sharedGameSync->B, 1, 0); // starts bloqued

   // Mutex
   sem_init(&sharedGameSync->C, 1, 1);
   sem_init(&sharedGameSync->D, 1, 1);
   sem_init(&sharedGameSync->E, 1, 1);

   // Contador de lectores
   sharedGameSync->F = 0;

   // Players: They start enabled (they can "play")
   for (int i = 0; i < 9; i++) {
      sem_init(&sharedGameSync->G[i], 1, 1);
   }
}
