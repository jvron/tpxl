#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tpxl/file.h"

TpxlFileType tpxl_detect_file_type(const char* path) {

    FILE* file = fopen(path, "rb");
    
    if (!file) {
        return TPXL_FILE_UNKNOWN;
    }

    uint8_t header[16];

    size_t read = fread(header, 1, sizeof(header), file);

    fclose(file);

    if (read >= 6) {
        if (memcmp(header, "GIF87a", 6) == 0 || memcmp(header, "GIF89a", 6) == 0) {
            return  TPXL_FILE_GIF;
        } 
    }

    if (read >= 8) {
        if (memcmp(header, "\x89PNG\r\n\x1a\n", 8) == 0) {
            return TPXL_FILE_PNG;
        }
    }

    if (read >= 3) {
        if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
            return TPXL_FILE_JPEG;
        }
    }

    return TPXL_FILE_UNKNOWN;
}

