
#include <time.h>
#include <unistd.h>

#include "time_util.h"

static time_p fps_last_time = {0};

void time_init() {
    clock_gettime(CLOCK_MONOTONIC, (struct timespec*)&fps_last_time);
}

void time_sleep_ms(int milliseconds) {
    struct timespec ts = {.tv_sec = milliseconds / 1000,
                          .tv_nsec = (milliseconds % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

double time_diff_sec(time_p start, time_p end) {
    return (double)(end.sec - start.sec) + (end.nsec - start.nsec) / 1e9;
};

time_p time_now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (time_p){.sec = ts.tv_sec, .nsec = ts.tv_nsec};
}
