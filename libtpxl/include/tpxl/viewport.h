#ifndef TPXL_VIEWPORT_H
#define TPXL_VIEWPORT_H

#include <stdint.h>

#include "type.h"

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;

} TpxlViewport;

TpxlResult tpxl_init_viewport(TpxlViewport* viewport);
TpxlResult tpxl_viewport_fit(TpxlViewport* viewport, uint32_t area_width, uint32_t area_height, uint32_t content_width, uint32_t content_height);

#endif
