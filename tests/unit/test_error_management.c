#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "CuTest.h"
#include "test_suites.h"

#include <error_management.h>

/*
 * manage_error writes human readable descriptions to stderr. We capture
 * whatever it writes by redirecting fileno(stderr) through a pipe, flushing
 * after the call and reading the pipe back into a local buffer.
 *
 * The pipe trick keeps the captured output self contained per test and
 * avoids any dependency on temporary files on disk. A non blocking read is
 * used so the test does not hang if manage_error prints nothing.
 */
static const int32_t capture_buffer_size = 4096;

typedef struct {
    int32_t saved_fd;
    int32_t read_fd;
    int32_t write_fd;
} stderr_capture_t;

static int32_t stderr_capture_begin(stderr_capture_t *capture) {
    int32_t pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        return -1;
    }

    /* Make the read end non blocking so we can drain without blocking. */
    int32_t flags = fcntl(pipe_fds[0], F_GETFL, 0);
    if (flags == -1 || fcntl(pipe_fds[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    fflush(stderr);
    capture->saved_fd = dup(fileno(stderr));
    if (capture->saved_fd == -1) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    if (dup2(pipe_fds[1], fileno(stderr)) == -1) {
        close(capture->saved_fd);
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    capture->read_fd = pipe_fds[0];
    capture->write_fd = pipe_fds[1];
    return 0;
}

static int32_t stderr_capture_end(stderr_capture_t *capture, char *out, int32_t out_size) {
    fflush(stderr);
    /* Restore the original stderr file descriptor. */
    dup2(capture->saved_fd, fileno(stderr));
    close(capture->saved_fd);
    /* Closing the write end lets the reader observe EOF if needed. */
    close(capture->write_fd);

    int32_t total = 0;
    while (total < out_size - 1) {
        ssize_t n = read(capture->read_fd, out + total, out_size - 1 - total);
        if (n <= 0) {
            break;
        }
        total += (int32_t)n;
    }
    out[total] = '\0';
    close(capture->read_fd);
    return total;
}

static void run_manage_error_and_capture(error_code_t code, char *out, int32_t out_size) {
    stderr_capture_t capture;
    if (stderr_capture_begin(&capture) != 0) {
        out[0] = '\0';
        return;
    }

    trace_t internal = {"some_file.c", "some_func", 123};
    manage_error(internal, TRACE_NONE, code);

    stderr_capture_end(&capture, out, out_size);
}

/*
 * A known code such as access_error should produce a message containing a
 * human readable prefix and the location metadata we pass in.
 */
static void test_manage_error_access(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(access_error, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "Permission denied") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_func") != NULL);
    CuAssertTrue(tc, strstr(buffer, "123") != NULL);
}

static void test_manage_error_file_exists(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(file_exists_error, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "File Exists Error") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
}

static void test_manage_error_mapping(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(mapping_error, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "mmap failed") != NULL);
}

static void test_manage_error_connection(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(connection_error, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "Failed to connect to shared memory") != NULL);
}

/*
 * Any code not explicitly handled in the switch should fall through to the
 * default branch and emit the generic "Unknown Error" message.
 */
static void test_manage_error_unknown_default(CuTest *tc) {
    char buffer[capture_buffer_size];
    /* 0xFFFF is well outside the handled set and the errno constants. */
    run_manage_error_and_capture((error_code_t)0xFFFF, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "Unknown error") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
}

/*
 * Every message flavor should mention the file name that was passed to
 * manage_error, since that is part of the shared _error_description
 * template. This is the invariant we rely on across all codes.
 */
static void test_manage_error_always_includes_location(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(access_error, buffer, capture_buffer_size);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_func") != NULL);

    run_manage_error_and_capture(mapping_error, buffer, capture_buffer_size);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_func") != NULL);
}

/*
 * invalid_argument_error must produce the "Invalid argument" description.
 */
static void test_manage_error_invalid_argument(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(invalid_argument_error, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "Invalid argument") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
}

/*
 * range_error must produce the "Value out of range" description.
 */
static void test_manage_error_range(CuTest *tc) {
    char buffer[capture_buffer_size];
    run_manage_error_and_capture(range_error, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "Value out of range") != NULL);
    CuAssertTrue(tc, strstr(buffer, "some_file.c") != NULL);
}

/*
 * manage_error must return the same error_code_t that was passed in.
 * This is the contract that callers like game_register_all rely on.
 */
static void test_manage_error_returns_same_code(CuTest *tc) {
    stderr_capture_t capture;
    if (stderr_capture_begin(&capture) != 0) {
        CuAssertTrue(tc, 0); /* Cannot proceed without capture */
        return;
    }

    trace_t internal = {"file.c", "func", 1};
    error_code_t result = manage_error(internal, TRACE_NONE, mapping_error);

    char discard[capture_buffer_size];
    stderr_capture_end(&capture, discard, capture_buffer_size);

    CuAssertIntEquals(tc, (int)mapping_error, (int)result);
}

/*
 * When both traces are provided, both "internal" and "caller" frames
 * should appear in the output.
 */
static void test_manage_error_both_traces(CuTest *tc) {
    stderr_capture_t capture;
    if (stderr_capture_begin(&capture) != 0) {
        CuAssertTrue(tc, 0);
        return;
    }

    trace_t internal = {"internal.c", "inner_func", 42};
    trace_t caller = {"caller.c", "outer_func", 99};
    manage_error(internal, caller, access_error);

    char buffer[capture_buffer_size];
    stderr_capture_end(&capture, buffer, capture_buffer_size);

    CuAssertTrue(tc, strstr(buffer, "internal.c") != NULL);
    CuAssertTrue(tc, strstr(buffer, "inner_func") != NULL);
    CuAssertTrue(tc, strstr(buffer, "42") != NULL);
    CuAssertTrue(tc, strstr(buffer, "caller.c") != NULL);
    CuAssertTrue(tc, strstr(buffer, "outer_func") != NULL);
    CuAssertTrue(tc, strstr(buffer, "99") != NULL);
}

/*
 * When both traces are TRACE_NONE, no frame lines should appear; only
 * the "Error: ..." header line. We verify by checking that "internal"
 * and "caller" labels are absent.
 */
static void test_manage_error_no_traces(CuTest *tc) {
    stderr_capture_t capture;
    if (stderr_capture_begin(&capture) != 0) {
        CuAssertTrue(tc, 0);
        return;
    }

    manage_error(TRACE_NONE, TRACE_NONE, connection_error);

    char buffer[capture_buffer_size];
    stderr_capture_end(&capture, buffer, capture_buffer_size);

    /* The error message itself must appear. */
    CuAssertTrue(tc, strstr(buffer, "Failed to connect to shared memory") != NULL);
    /* No frame labels should be present. */
    CuAssertTrue(tc, strstr(buffer, "internal") == NULL);
    CuAssertTrue(tc, strstr(buffer, "caller") == NULL);
}

CuSuite *error_management_get_suite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_manage_error_access);
    SUITE_ADD_TEST(suite, test_manage_error_file_exists);
    SUITE_ADD_TEST(suite, test_manage_error_mapping);
    SUITE_ADD_TEST(suite, test_manage_error_connection);
    SUITE_ADD_TEST(suite, test_manage_error_unknown_default);
    SUITE_ADD_TEST(suite, test_manage_error_always_includes_location);
    SUITE_ADD_TEST(suite, test_manage_error_invalid_argument);
    SUITE_ADD_TEST(suite, test_manage_error_range);
    SUITE_ADD_TEST(suite, test_manage_error_returns_same_code);
    SUITE_ADD_TEST(suite, test_manage_error_both_traces);
    SUITE_ADD_TEST(suite, test_manage_error_no_traces);
    return suite;
}
