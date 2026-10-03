#include <stddef.h>

#include "tpxl/type.h"

static const char* result_to_string[] = {
    [TPXL_OK] = "success",

    [TPXL_INVALID_FILE] = "unsupported or invalid file format",
    [TPXL_INVALID_ARGUMENT] = "invalid argument",
    [TPXL_INVALID_FORMAT] = "invalid format",
    [TPXL_INVALID_BACKEND] = "invalid renderer backend",
    [TPXL_NOT_FOUND] = "not found",

    [TPXL_LOAD_FAILED] = "loading failed",
    [TPXL_RESIZE_FAILED] = "resizing failed",
    [TPXL_DECODE_FAILED] = "decoding failed",

    [TPXL_BACKEND_CREATION_FAILED] = "renderer backend creation failed",

    [TPXL_EOF] = "end of file",

    [TPXL_NEED_PACKET] = "decoder needs another packet",

    [TPXL_PLAYER_CREATION_FAILED] = "player creation failed",
    [TPXL_PLAYING_FAILED] = "playing failed",
    [TPXL_PAUSING_FAILED] = "pausing failed",
    [TPXL_MUTE_FAILED] = "muting failed",
    [TPXL_UNMUTE_FAILED] = "unmuting failed",

    [TPXL_THREAD_CREATION_ERROR] = "thread creation error",

    [TPXL_SHUTDOWN] = "player thread shutdown",

    [TPXL_QUEUE_CLOSED] = "queue closed",
    [TPXL_QUEUE_EMPTY] = "queue empty",

    [TPXL_MAP_EMPTY] = "map empty",
    [TPXL_MAP_FULL] = "map full",
    [TPXL_MAP_NOT_FOUND] = "map entry not found",
    [TPXL_MAP_INVALID_ID] = "invalid map ID",

    [TPXL_UNSUPPORTED_FORMAT] = "unsupported format",
    [TPXL_OUT_OF_MEMORY] = "out of memory",
    [TPXL_ENCODING_FAILED] = "encoding failed",
    [TPXL_COMPRESSION_FAILED] = "compression failed",
    [TPXL_RENDER_FAILED] = "rendering failed",
    [TPXL_IO_ERROR] = "I/O error",

    [TPXL_ERROR] = "internal error",
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
        case TPXL_FORMAT_R: return 1;
        case TPXL_FORMAT_RG: return 2;
        case TPXL_FORMAT_RGB: return 3;
        case TPXL_FORMAT_RGBA: return 4;
        default:
            return 0;
    }
}

const char* tpxl_format_to_string(TpxlFormat format) {

    if (format >= TPXL_FORMAT_COUNT) {
        return NULL;
    }
    return format_to_string[format];
}  

const char* tpxl_result_to_string(TpxlResult result) {

    if (result >= TPXL_RESULT_COUNT) {
        return NULL;
    }
    return result_to_string[result];
}
