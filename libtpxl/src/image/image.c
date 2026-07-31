#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/image.h"

TpxlResult tpxl_load_image(const char* file, TpxlImage* image) {

    unsigned char* pixels = stbi_load(file, &image->width, &image->height, &image->channels, 0);

    if (!pixels) {
        image->pixels = NULL;
        return TPXL_ERROR;
    }

    image->pixels = pixels;

    return TPXL_OK;
}

void tpxl_free_image(TpxlImage* image) {

    if (!image) {
        return;
    }
    stbi_image_free(image->pixels);

    image->width = 0;
    image->height = 0;
    image->channels = 0;
    image->pixels = NULL;
}

