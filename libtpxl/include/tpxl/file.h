#ifndef TPXL_FILE_H
#define TPXL_FILE_H

typedef enum {
    TPXL_FILE_UNKNOWN = 0,
    TPXL_FILE_PNG,
    TPXL_FILE_JPEG,
    TPXL_FILE_GIF,
    TPXL_FILE_VIDEO

} TpxlFileType;

TpxlFileType tpxl_detect_file_type(const char* path);

#endif
