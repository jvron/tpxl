#include <stddef.h>

#include "tpxl/type.h"

static const char* result_to_string[] = {
    [TPXL_OK] = "success",
    [TPXL_INVALID_ARGUMENT] = "invalid argument",
    [TPXL_INVALID_FORMAT] = "invalid format",
    [TPXL_IMAGE_LOAD_FAILED] = "image loading failed",
    [TPXL_UNSUPPORTED_FORMAT] = "unsupported format",
    [TPXL_OUT_OF_MEMORY] = "out of memory",
    [TPXL_ENCODING_FAILED] = "encoding failed",
    [TPXL_RENDER_FAILED] = "rendering failed",
    [TPXL_IO_ERROR] = "I/O error",
    [TPXL_ERROR] = "error",
};

static const char* format_to_string[] = {
    [TPXL_FORMAT_UNKNOWN] = "unknown",
    [TPXL_FORMAT_R] = "r",
    [TPXL_FORMAT_RG] = "rg",
    [TPXL_FORMAT_RGB] = "rgb",
    [TPXL_FORMAT_RGBA] = "rgba",
};

int tpxl_format_to_channels(TpxlFormat format) {
    switch (format) {
        case TPXL_FORMAT_R:
            return 1;
        case TPXL_FORMAT_RG:
            return 2;
        case TPXL_FORMAT_RGB:
            return 3;
        case TPXL_FORMAT_RGBA:
            return 4;
        default:
            return 0;
    }
}

const char* tpxl_format_to_string(TpxlFormat format) {
    if (format > TPXL_FORMAT_COUNT) {
        return NULL;
    }
    return format_to_string[format];
}  

const char* tpxl_result_to_string(TpxlResult result) {
    if (result > TPXL_RESULT_COUNT) {
        return NULL;
    }
    return result_to_string[result];
}
