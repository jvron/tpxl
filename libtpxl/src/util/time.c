#include <stdint.h>

#include "tpxl/util.h"

#ifdef _WIN32

#include <windows.h>

uint64_t tpxl_get_time_ms(void) {

    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;

    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);

    return (uint64_t)counter.QuadPart * 1000 / frequency.QuadPart;
}

void tpxl_sleep_ms(uint32_t milliseconds) {
    Sleep(milliseconds);
}

void tpxl_sleep_us(uint64_t microseconds) {
    
    DWORD milliseconds = (DWORD)((microseconds + 999) / 1000);
    Sleep(milliseconds);
}

#else

#define _POSIX_C_SOURCE 200809L

#include <time.h>

uint64_t tpxl_get_time_ms(void) {

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

void tpxl_sleep_ms(uint32_t milliseconds) {

    struct timespec ts = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (long)(milliseconds % 1000) * 1000000L
    };

    nanosleep(&ts, NULL);
}

void tpxl_sleep_us(uint64_t microseconds) {
    
    struct timespec ts = {
        .tv_sec = microseconds / 1000000,
        .tv_nsec = (long)(microseconds % 1000000) * 1000L
    };

    nanosleep(&ts, NULL);
}

#endif
