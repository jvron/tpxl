#ifndef TPXL_TYPE_H
#define TPXL_TYPE_H

#include <stdint.h>

typedef struct {
    int width;
    int height;
    int channels;
    uint8_t* pixels;
    
} TpxlImage;

typedef enum {
    TPXL_OK,
    TPXL_ERROR,

} TpxlResult;

#endif
