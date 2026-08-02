#ifndef TPXL_TYPE_H
#define TPXL_TYPE_H

#include <stdint.h>
#include <stdio.h>

typedef enum {
    TPXL_FORMAT_UNKNOWN = 0,
    TPXL_FORMAT_R, 
    TPXL_FORMAT_RG,
    TPXL_FORMAT_RGB,
    TPXL_FORMAT_RGBA,

    TPXL_FORMAT_COUNT,

} TpxlFormat;

typedef struct {
    int width;
    int height;
    TpxlFormat format;
    uint8_t* pixels;

} TpxlImage;

typedef enum {
    TPXL_OK = 0,
    TPXL_INVALID_ARGUMENT,
    TPXL_INVALID_FORMAT,
    TPXL_IMAGE_LOAD_FAILED,
    TPXL_UNSUPPORTED_FORMAT,
    TPXL_OUT_OF_MEMORY,
    TPXL_ENCODING_FAILED,
    TPXL_RENDER_FAILED,
    TPXL_IO_ERROR,
    TPXL_ERROR,

    TPXL_RESULT_COUNT,

} TpxlResult;

typedef struct {
    TpxlImage* frames;
    uint32_t* delays;
    size_t count;
    
} TpxlAnimation;

int tpxl_format_to_channels(TpxlFormat format);
const char* tpxl_format_to_string(TpxlFormat format);
const char* tpxl_result_to_string(TpxlResult result);

#endif
