#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kitty.h"
#include "tpxl/type.h"
#include "util/base64.h"

TpxlResult tpxl_kitty_render(TpxlContext* context, TpxlImage* image, TpxlMediaType media_type) {

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
            free(data);
            return TPXL_UNSUPPORTED_FORMAT;
    }

    int cursor_policy = 0;

    switch (media_type) {
        case TPXL_MEDIA_STILL:
            cursor_policy = 0;
            break;
        case TPXL_MEDIA_ANIMATED:
            cursor_policy = 1;
            break;
    }

    // convert viewport dimensions to terminal cells
    uint32_t columns = (context->viewport.width + context->terminal.cell_width - 1) / context->terminal.cell_width;
    uint32_t rows = (context->viewport.height + context->terminal.cell_height - 1) / context->terminal.cell_height;

    // sub-cell offset
    uint32_t offset_x = context->viewport.x % context->terminal.cell_width;
    uint32_t offset_y = context->viewport.y % context->terminal.cell_height;

    // convert viewport x and y to cells
    uint32_t cell_x = context->viewport.x / context->terminal.cell_width;
    uint32_t cell_y = context->viewport.y / context->terminal.cell_height;

    uint32_t target_column = context->terminal.cursor_column + cell_x;
    uint32_t target_row = context->terminal.cursor_row + cell_y;

    // move cursor
    fprintf(stdout,"\033[%u;%uH", target_row, target_column);

    const size_t output_length = strlen(data);
    const size_t CHUNK_SIZE = 4096;

    for (size_t i = 0; i < output_length; i += CHUNK_SIZE) {

        char* chunk = data + i;

        size_t remaining = output_length - i;

        size_t chunk_length = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        int more_chunks = remaining > CHUNK_SIZE ? 1 : 0;

        fprintf(stdout, 
            "\x1b_Ga=T,"
            "f=%d,"
            "s=%d,"
            "v=%d,"
            "X=%u,"
            "Y=%u,"
            "c=%u,"
            "r=%u,"
            "C=%d,"
            "m=%d;",
            kitty_format,
            image->width,
            image->height,
            offset_x,
            offset_y, 
            columns,
            rows,
            cursor_policy,
            more_chunks
        );

        fwrite(chunk, 1, chunk_length, stdout);
        fprintf(stdout, "\x1b\\");
    }

    free(data);

    return TPXL_OK;
}
