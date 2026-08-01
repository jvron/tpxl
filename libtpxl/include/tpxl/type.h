#ifndef TPXL_TYPE_H
#define TPXL_TYPE_H

#include <stdint.h>

typedef enum {
    TPXL_FORMAT_UNKNOWN = 0,
    TPXL_FORMAT_R, 
    TPXL_FORMAT_RG,
    TPXL_FORMAT_RGB,
    TPXL_FORMAT_RGBA,

} TpxlFormat;

typedef struct {
    int width;
    int height;
    TpxlFormat format;
    uint8_t* pixels;

} TpxlImage;

typedef enum {
    TPXL_OK,
    TPXL_ERROR,

} TpxlResult;

#endif
