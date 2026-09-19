#ifndef TPXL_UTIL_H
#define TPXL_UTIL_H

#include <stdint.h>

uint64_t tpxl_get_time_ms(void);
void tpxl_sleep_ms(uint32_t milliseconds);
void tpxl_sleep_us(uint64_t microseconds);

#endif
