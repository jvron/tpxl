#ifndef TPXL_FILE_H
#define TPXL_FILE_H

typedef enum {
    TPXL_FILE_UNKNOWN = 0,
    TPXL_FILE_PNG,
    TPXL_FILE_JPEG,

    TPXL_FILE_GIF,

    TPXL_FILE_MP4,
    TPXL_FILE_MKV,
    TPXL_FILE_AVI,
    TPXL_FILE_MOV,
    
    TPXL_FILE_MP3,
    TPXL_FILE_FLAC,
    TPXL_FILE_WAV,
    TPXL_FILE_OGG,
} TpxlFileType;

TpxlFileType tpxl_detect_file_type(const char* path);

#endif
