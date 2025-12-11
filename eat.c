#include "eat.h"
#include "timespec_operations.h" /* for incr_timespec */

void inline eat(const struct timespec *cpu_time)
{
    int err;
    struct timespec current_time, time_to_go;

    err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &current_time);
    add_timespec(&time_to_go, &current_time, cpu_time);

    while (smaller_timespec(&current_time, &time_to_go)) {
        err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &current_time);
    }
}

