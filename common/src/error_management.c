#include <error_management.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>

#define _error_description "Error produced at File: %s\n Function: %s Line: %llu\n"

void manage_errno(const char *file, const char *func, uint64_t line, error_code_t code) {
   switch (code) {
   case access_error:
      fprintf(stderr, "Permission denied, access error...\n" _error_description, file, func, line);
      break;
   case file_exists_error:
      fprintf(stderr, "File Exists Error...\n" _error_description, file, func, line);
      break;
   case mapping_error:
      fprintf(stderr, "mmap failed...\n" _error_description, file, func, line);
   default: /* We log every single error independently from its code */
      fprintf(stderr, "Unknown Error...\n" _error_description, file, func, line);
   }
}

void clear_error() {
   errno = 0;
}
