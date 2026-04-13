#include <error_management.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

static const char *description_for(error_code_t code) {
    switch (code) {
    case access_error:
        return "Permission denied, access error";
    case file_exists_error:
        return "File Exists Error";
    case mapping_error:
        return "mmap failed";
    case connection_error:
        return "Failed to connect to shared memory";
    case invalid_argument_error:
        return "Invalid argument";
    case range_error:
        return "Value out of range";
    case unreachable:
        return "Unreachable code reached";
    case no_memory_error:
        return "Cannot allocate memory";
    case no_entry_error:
        return "No such file or directory";
    case bad_fd_error:
        return "Bad file descriptor";
    case io_error:
        return "Input/output error";
    case broken_pipe_error:
        return "Broken pipe";
    case interrupted_error:
        return "Interrupted system call";
    case too_many_files_error:
        return "Too many open files";
    case file_table_full_error:
        return "Too many open files in system";
    case no_space_error:
        return "No space left on device";
    case resource_busy_error:
        return "Resource temporarily unavailable";
    case permission_error:
        return "Operation not permitted";
    case exec_format_error:
        return "Exec format error";
    default:
        return "Unknown error";
    }
}

static void print_frame(const char *label, trace_t frame) {
    if (frame.file == NULL) {
        return;
    }
    fprintf(stderr, "  %s: %s:%s:%" PRIu64 "\n", label, frame.file, frame.func, (uint64_t)frame.line);
}

error_code_t manage_error(trace_t internal, trace_t caller, error_code_t code) {
    fprintf(stderr, "Error: %s\n", description_for(code));
    print_frame("internal", internal);
    print_frame("caller  ", caller);
    return code;
}
