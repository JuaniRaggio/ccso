#include <error_management.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
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
    default:
        return "Unknown Error";
    }
}

static void print_frame(const char *label, trace_t frame) {
    if (frame.file == NULL) {
        return;
    }
    fprintf(stderr, "  %s: %s:%s:%llu\n", label, frame.file, frame.func, (uint64_t)frame.line);
}

error_code_t manage_error(trace_t internal, trace_t caller, error_code_t code) {
    fprintf(stderr, "Error: %s\n", description_for(code));
    print_frame("internal", internal);
    print_frame("caller  ", caller);
    return code;
}
