#include "signals.h"
#include <signal.h>
#include <stdint.h>

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    should_exit = 1;
}

void setup_signals(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

bool was_interrupted(void) {
    return should_exit != 0;
}
