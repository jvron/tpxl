#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kitty.h"
#include "tpxl/type.h"
#include "util/base64.h"

TpxlResult tpxl_kitty_render(TpxlImage* image) {

    if (!image) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (image->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    int channels = tpxl_format_to_channels(image->format);

    size_t length = image->width * image->height * channels;

    char* data = malloc(tpxl_base64_encoded_size(length));

    if (!data) {
        return TPXL_OUT_OF_MEMORY;
    }

    if (tpxl_base64_encode(image->pixels, length, data) != TPXL_OK) {
        free(data);
        return TPXL_ENCODING_FAILED;
    }

    int kitty_format = 0;

    switch (image->format) {

        case TPXL_FORMAT_RGB:
            kitty_format = 24;
            break;
        case TPXL_FORMAT_RGBA:
            kitty_format = 32;
            break;
        default:
            return TPXL_UNSUPPORTED_FORMAT;
    }

    const size_t output_length = strlen(data);
    const size_t CHUNK_SIZE = 4096;

    for (size_t i = 0; i < output_length; i += CHUNK_SIZE) {

        char* chunk = data + i;

        size_t remaining = output_length - i;

        size_t chunk_length = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        int more_chunks = remaining > CHUNK_SIZE ? 1 : 0;

        fprintf(stdout, "\x1b_Ga=T,");
        fprintf(stdout, "f=%d,s=%d,v=%d,m=%d;", kitty_format, image->width, image->height, more_chunks);
        fwrite(chunk, 1, chunk_length, stdout);
        fprintf(stdout, "\x1b\\");
    }

    printf("\n");

    free(data);

    return TPXL_OK;
}
