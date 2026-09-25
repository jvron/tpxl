#include "tpxl/type.h"

TpxlResult tpxl_scale_fit(
    uint32_t area_width,
    uint32_t area_height,
    uint32_t content_width,
    uint32_t content_height,
    uint32_t* scaled_width,
    uint32_t* scaled_height
) {
    if (!area_width || !area_height || !content_width || !content_height || !scaled_width || !scaled_height) {
        return TPXL_INVALID_ARGUMENT;
    }

    float scale_x = (float)area_width / content_width;
    float scale_y = (float)area_height / content_height;

    float scale = scale_x < scale_y ? scale_x : scale_y;

    *scaled_width = (uint32_t)(content_width * scale);
    *scaled_height = (uint32_t)(content_height * scale);

    return TPXL_OK;
}