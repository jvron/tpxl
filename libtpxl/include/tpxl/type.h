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
    uint32_t width;
    uint32_t height;
    TpxlFormat format;
    uint8_t* pixels;

} TpxlImage;

typedef enum {
    TPXL_OK = 0,
    TPXL_INVALID_FILE,
    TPXL_INVALID_ARGUMENT,
    TPXL_INVALID_FORMAT,
    TPXL_IMAGE_LOAD_FAILED,
    TPXL_GIF_LOAD_FAILED,
    TPXL_VIDEO_LOAD_FAILED,
    TPXL_EOF,
    TPXL_VIDEO_NEED_PACKET,
    TPXL_VIDEO_DECODE_FAILED,
    TPXL_FRAME_QUEUE_FULL,
    TPXL_FRAME_QUEUE_EMPTY,
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

typedef enum {
    TPXL_MEDIA_STILL = 0,
    TPXL_MEDIA_ANIMATED,

    TPXL_MEDIA_TYPE_COUNT
    
} TpxlMediaType;

int tpxl_format_to_channels(TpxlFormat format);
const char* tpxl_format_to_string(TpxlFormat format);
const char* tpxl_result_to_string(TpxlResult result);

#endif
