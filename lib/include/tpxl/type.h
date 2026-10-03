#ifndef TPXL_TYPE_H
#define TPXL_TYPE_H

#include <stdint.h>

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
    TPXL_INVALID_BACKEND,
    TPXL_NOT_FOUND,

    TPXL_LOAD_FAILED,
    TPXL_DECODE_FAILED,
    TPXL_RESIZE_FAILED,

    TPXL_BACKEND_CREATION_FAILED,

    TPXL_EOF,

    TPXL_NEED_PACKET,

    TPXL_PLAYER_CREATION_FAILED,
    TPXL_PLAYING_FAILED,
    TPXL_PAUSING_FAILED,
    TPXL_MUTE_FAILED,
    TPXL_UNMUTE_FAILED,

    TPXL_SHUTDOWN,

    TPXL_QUEUE_CLOSED,
    TPXL_QUEUE_EMPTY,

    TPXL_MAP_EMPTY,
    TPXL_MAP_FULL,
    TPXL_MAP_NOT_FOUND,
    TPXL_MAP_INVALID_ID,

    TPXL_THREAD_CREATION_ERROR,

    TPXL_UNSUPPORTED_FORMAT,
    TPXL_OUT_OF_MEMORY,
    TPXL_ENCODING_FAILED,
    TPXL_COMPRESSION_FAILED,
    TPXL_RENDER_FAILED,
    TPXL_IO_ERROR,

    TPXL_ERROR,

    TPXL_RESULT_COUNT,
} TpxlResult;

typedef enum {
    TPXL_MEDIA_UNKNOWN = 0,
    TPXL_MEDIA_IMAGE,
    TPXL_MEDIA_ANIMATION,
    TPXL_MEDIA_VIDEO,
    TPXL_MEDIA_AUDIO,
    TPXL_MEDIA_TYPE_COUNT
} TpxlMediaType;

int tpxl_format_to_channels(TpxlFormat format);
const char* tpxl_format_to_string(TpxlFormat format);
const char* tpxl_result_to_string(TpxlResult result);

#endif
