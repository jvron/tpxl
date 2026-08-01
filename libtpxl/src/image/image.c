#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/image.h"

TpxlResult tpxl_load_image(const char* path, TpxlImage* image) {

    if (!path || !image) {
        return TPXL_ERROR;
    }

    int channels = 0;
    unsigned char* pixels = stbi_load(path, &image->width, &image->height, &channels, 0);

    if (!pixels) {
        image->width = 0;
        image->height = 0;
        image->format = TPXL_FORMAT_UNKNOWN;
        image->pixels = NULL;

        return TPXL_ERROR;
    }

    image->pixels = pixels;

    switch (channels) {
        case 1:
            image->format = TPXL_FORMAT_R;
            break;
        case 2:
            image->format = TPXL_FORMAT_RG;
            break;
        case 3:
            image->format = TPXL_FORMAT_RGB;
            break;
        case 4:
            image->format = TPXL_FORMAT_RGBA;
            break;

        default:
            tpxl_free_image(image);
            return TPXL_ERROR;
    }

    return TPXL_OK;
}

void tpxl_free_image(TpxlImage* image) {

    if (!image) {
        return;
    }
    stbi_image_free(image->pixels);

    image->width = 0;
    image->height = 0;
    image->format = TPXL_FORMAT_UNKNOWN;
    image->pixels = NULL;
}

