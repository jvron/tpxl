#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize2.h>

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

TpxlResult tpxl_resize_image(TpxlImage* image, uint32_t output_width, uint32_t output_height) {

    if (!image) {
        return TPXL_INVALID_ARGUMENT;
    }

    stbir_pixel_layout layout = STBIR_RGB;

    switch (image->format) {
        case TPXL_FORMAT_R:
            layout = STBIR_1CHANNEL;
        case TPXL_FORMAT_RG:
            layout = STBIR_2CHANNEL;
            break;
        case TPXL_FORMAT_RGB:
            layout = STBIR_RGB;
            break;
        case TPXL_FORMAT_RGBA:
            layout = STBIR_RGBA;
            break;

        default:
            return TPXL_INVALID_FORMAT;
    }

    size_t output_size = output_width * output_height * tpxl_format_to_channels(image->format);
    uint8_t* output_buffer = malloc(output_size);

    if (!output_buffer) {
        return TPXL_OUT_OF_MEMORY;
    }

    void* resized_pixels = stbir_resize(
        image->pixels, 
        image->width, 
        image->height, 
        0, 
        output_buffer, 
        output_width, 
        output_height, 
        0, 
        layout, 
        STBIR_TYPE_UINT8, 
        STBIR_EDGE_CLAMP, 
        STBIR_FILTER_CUBICBSPLINE
    );

    if (!resized_pixels) {
        free(output_buffer);
        return TPXL_IMAGE_RESIZE_FAILED;
    }

    stbi_image_free(image->pixels);

    image->width = output_width;
    image->height = output_height;
    image->pixels = (uint8_t*)resized_pixels;

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
