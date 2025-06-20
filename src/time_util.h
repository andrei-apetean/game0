#pragma once

#include <stdint.h>
typedef struct {
    int64_t sec;
    int64_t nsec;
} time_p;

void   time_init();
void   time_sleep_ms(int milliseconds);
double time_diff_sec(time_p start, time_p end);

time_p time_now();
