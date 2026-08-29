#define _POSIX_C_SOURCE 200809L

#include "tpxl/util.h"

#ifdef _WIN32

#include <windows.h>

void tpxl_sleep_ms(uint32_t milliseconds) {

    Sleep(milliseconds);
}

void tpxl_sleep_us(uint64_t microseconds) {
    
    DWORD milliseconds = (DWORD)((microseconds + 999) / 1000);
    Sleep(milliseconds);
}

#else

#include <time.h>

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
