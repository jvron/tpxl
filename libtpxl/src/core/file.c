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

    // MP4 / MOV
    if (read >= 12 && memcmp(header + 4, "ftyp", 4) == 0) {
        return TPXL_FILE_VIDEO;
    }

    // MKV / WebM
    if (read >= 4 && memcmp(header, "\x1A\x45\xDF\xA3", 4) == 0) {
        return TPXL_FILE_VIDEO;
    }
    
    // AVI 
    if (read >= 12 && memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "AVI ", 4) == 0) {
        return TPXL_FILE_VIDEO;
    }

    // MP3
    if (read >= 3) {
        if (memcmp(header, "ID3", 3) == 0) {
            return TPXL_FILE_AUDIO;
        }

        if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) {
            return TPXL_FILE_AUDIO;        
        }
    }

    // FLAC
    if (read >= 4 && memcmp(header, "fLaC", 4) == 0) {
        return TPXL_FILE_AUDIO; 
    }

    // WAV
    if (read >= 12 && memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "WAVE", 4) == 0) {
        return TPXL_FILE_AUDIO; 
    }

    // OGG
    if (read >= 4 && memcmp(header, "OggS", 4) == 0) {
        return TPXL_FILE_AUDIO;
    }

    return TPXL_FILE_UNKNOWN;
}

