#ifndef TPXL_UTIL_H
#define TPXL_UTIL_H

#include <stdint.h>

#include "type.h"

typedef enum {
    TPXL_SCALE_NONE = 0,
    TPXL_SCALE_FIT,
    TPXL_SCALE_FILL,
    TPXL_SCALE_COUNT,
} TpxlScaleMode;

uint64_t tpxl_get_time_ms(void);
void tpxl_sleep_ms(uint32_t milliseconds);
void tpxl_sleep_us(uint64_t microseconds);

TpxlResult tpxl_scale_fit(
    uint32_t area_width,
    uint32_t area_height,
    uint32_t content_width,
    uint32_t content_height,
    uint32_t* scaled_width,
    uint32_t* scaled_height
);

#endif
