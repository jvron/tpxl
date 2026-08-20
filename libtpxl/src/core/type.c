#include <stddef.h>

#include "tpxl/type.h"

static const char* result_to_string[] = {
    [TPXL_OK] = "success",
    [TPXL_INVALID_FILE] = "unsupported or invalid file format",
    [TPXL_INVALID_ARGUMENT] = "invalid argument",
    [TPXL_INVALID_FORMAT] = "invalid format",
    [TPXL_IMAGE_LOAD_FAILED] = "image loading failed",
    [TPXL_GIF_LOAD_FAILED] = "gif loading failed",
    [TPXL_VIDEO_LOAD_FAILED] = "video loading failed",
    [TPXL_AUDIO_LOAD_FAILED] = "audio loading failed",
    [TPXL_EOF] = "end of file",
    [TPXL_VIDEO_DECODE_FAILED] = "video decoding failed",
    [TPXL_VIDEO_NEED_PACKET] = "video decoder needs another packet",
    [TPXL_VIDEO_PLAYER_CREATION_FAILED] = "video player creation failed",
    [TPXL_VIDEO_PLAYING_FAILED] = "video playing failed",
    [TPXL_AUDIO_PLAYER_CREATION_FAILED] = "audio player creation failed",
    [TPXL_AUDIO_DECODE_FAILED] = "audio decoding failed",
    [TPXL_AUDIO_NEED_PACKET] = "audio decoder needs another packet",
    [TPXL_AUDIO_PLAYING_FAILED] = "audio playing failed",
    [TPXL_THREAD_CREATION_ERROR] = "thread creation error",
    [TPXL_SHUTDOWN] = "player thread shutdown",
    [TPXL_UNSUPPORTED_FORMAT] = "unsupported format",
    [TPXL_OUT_OF_MEMORY] = "out of memory",
    [TPXL_ENCODING_FAILED] = "encoding failed",
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
