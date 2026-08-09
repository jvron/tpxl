#include "tpxl/viewport.h"
#include "tpxl/type.h"

TpxlResult tpxl_init_viewport(TpxlViewport* viewport) {
    
    if (!viewport) {
        return TPXL_INVALID_ARGUMENT;
    }

    viewport->width = 0;
    viewport->height = 0;
    viewport->x = 0;
    viewport->y = 0;

    return TPXL_OK;
}

TpxlResult tpxl_viewport_fit(TpxlViewport* viewport, uint32_t area_width, uint32_t area_height, uint32_t content_width, uint32_t content_height) {

    if (!viewport || !area_width || !area_height || !content_width || !content_height) {
        return TPXL_INVALID_ARGUMENT;
    }

    float scale_x = (float)area_width / content_width;
    float scale_y = (float)area_height / content_height;

    // scale to fit
    float scale = scale_x < scale_y ? scale_x : scale_y;

    // size to draw the image
    viewport->width = (uint32_t)content_width * scale;
    viewport->height = (uint32_t)content_height * scale;

    return TPXL_OK;
}
