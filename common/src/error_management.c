#include <error_management.h>

void manage_errno(const char *file, const char *func, uint64_t line) {
   switch (errno) {
   case EACCES:
      fprintf(stderr, "Access Error... File: %s\n Function: %s Line: %llu\n", file, func, line);
      break;
   case EEXIST:
      fprintf(stderr, "Exist Error... File: %s\n Function: %s Line: %llu\n", file, func, line);
      break;
   }
}

void clear_error() {
    errno = 0;
}
