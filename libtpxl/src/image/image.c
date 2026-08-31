#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdio.h>
#include <stdint.h>

#include "tpxl/type.h"
#include "tpxl/image.h"

TpxlResult tpxl_load_image(const char* file, TpxlImage* image) {

    if (!file || !image) {
        return TPXL_INVALID_ARGUMENT;
    }

    int channels = 0;
    int width = 0;
    int height = 0;
    unsigned char* pixels = stbi_load(file, &width, &height, &channels, 0);

    if (!pixels) {
        *image = (TpxlImage){0};
        return TPXL_IMAGE_LOAD_FAILED;
    }

    image->width = (uint32_t)width;
    image->height = (uint32_t)height;
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
            return TPXL_UNSUPPORTED_FORMAT;
    }

    return TPXL_OK;
}

void tpxl_free_image(TpxlImage* image) {

    if (!image) {
        return;
    }

    stbi_image_free(image->pixels);

    *image = (TpxlImage){0};
}

void tpxl_free_frame(TpxlImage* frame) {

    if (!frame) {
        return;
    }

    free(frame->pixels);
    
    *frame = (TpxlImage){0};
}

TpxlResult tpxl_print_image_info(TpxlImage* image) {
   
    if (!image) {
        return TPXL_INVALID_ARGUMENT;
    }

    printf("Width: %d\n", image->width);
    printf("Height: %d\n", image->height);
    printf("Format: %s\n", tpxl_format_to_string(image->format));
    printf("Channels: %d\n", tpxl_format_to_channels(image->format));

    return TPXL_OK;
}
