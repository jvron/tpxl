#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kitty.h"
#include "tpxl/type.h"
#include "util/base64.h"

const size_t CHUNK_SIZE = 4096;

TpxlResult tpxl_set_kitty_context(TpxlKittyContext* kitty_context, TpxlContext* context, TpxlMediaType media_type) {

    if (!kitty_context || !context){
        return TPXL_INVALID_ARGUMENT;
    }

    switch (media_type) {
        case TPXL_MEDIA_STILL:
            kitty_context->cursor_policy = 0;
            break;
        case TPXL_MEDIA_ANIMATED:
            kitty_context->cursor_policy = 1;
            break;
        default:
            return TPXL_INVALID_ARGUMENT;
    }

    // convert viewport dimensions to terminal cells
    kitty_context->columns = (context->viewport.width + context->terminal.cell_width - 1) / context->terminal.cell_width;
    kitty_context->rows = (context->viewport.height + context->terminal.cell_height - 1) / context->terminal.cell_height;

    // sub-cell offset
    kitty_context->offset_x = context->viewport.x % context->terminal.cell_width;
    kitty_context->offset_y = context->viewport.y % context->terminal.cell_height;

    // convert viewport x and y to cells
    kitty_context->cell_x = context->viewport.x / context->terminal.cell_width;
    kitty_context->cell_y = context->viewport.y / context->terminal.cell_height;

    kitty_context->target_column = context->terminal.cursor_column + kitty_context->cell_x;
    kitty_context->target_row = context->terminal.cursor_row + kitty_context->cell_y;

    return TPXL_OK;
}

TpxlResult tpxl_set_kitty_frame(TpxlKittyContext* kitty_context, uint32_t width, uint32_t height, TpxlFormat format) {

    if (!kitty_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (format) {
        case TPXL_FORMAT_RGB:
            kitty_context->kitty_format = 24;
            break;
        case TPXL_FORMAT_RGBA:
            kitty_context->kitty_format = 32;
            break;
        default:
            return TPXL_UNSUPPORTED_FORMAT;
    }

    size_t frame_size = width * height * tpxl_format_to_channels(format);

    if (kitty_context->encoded_data != NULL && kitty_context->frame_size == frame_size) {
        return TPXL_OK;
    }

    size_t encoded_length = tpxl_base64_encoded_size(frame_size);
    
    char* data = malloc(encoded_length);
    
    if (!data) {
        return TPXL_OUT_OF_MEMORY;
    }
    free(kitty_context->encoded_data);

    kitty_context->frame_size = frame_size;
    kitty_context->encoded_length = encoded_length;
    kitty_context->encoded_data = data;

    return TPXL_OK;
}

TpxlResult tpxl_kitty_render(TpxlKittyContext* kitty_context, TpxlImage* frame) {

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    size_t output_length = 0;

    if (tpxl_base64_encode(frame->pixels, kitty_context->frame_size, kitty_context->encoded_data, &output_length) != TPXL_OK) {
        return TPXL_ENCODING_FAILED;
    }

    // move cursor
    fprintf(stdout,"\033[%u;%uH", kitty_context->target_row, kitty_context->target_column);

    for (size_t i = 0; i < output_length; i += CHUNK_SIZE) {

        char* chunk = kitty_context->encoded_data + i;

        size_t remaining = output_length - i;

        size_t chunk_length = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        int more_chunks = remaining > CHUNK_SIZE ? 1 : 0;

        if (i == 0) {
            fprintf(
                stdout,
                "\x1b_Ga=T,"
                "f=%d,"
                "s=%u,"
                "v=%u,"
                "X=%u,"
                "Y=%u,"
                "c=%u,"
                "r=%u,"
                "C=%d,"
                "m=%d;",
                kitty_context->kitty_format,
                frame->width,
                frame->height,
                kitty_context->offset_x,
                kitty_context->offset_y, 
                kitty_context->columns,
                kitty_context->rows,
                kitty_context->cursor_policy,
                more_chunks
            );
        }
        else {
            fprintf(
                stdout,
                "\x1b_Gm=%d;",
                more_chunks
            );
        }

        fwrite(chunk, 1, chunk_length, stdout);
        fprintf(stdout, "\x1b\\");
    }
    fflush(stdout);

    return TPXL_OK;
}

TpxlResult tpxl_kitty_transmit(TpxlKittyContext* kitty_context, TpxlImage* frame, uint32_t frame_id) {

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    size_t output_length = 0;
    if (tpxl_base64_encode(frame->pixels, kitty_context->frame_size, kitty_context->encoded_data, &output_length) != TPXL_OK) {
        return TPXL_ENCODING_FAILED;
    }

    // move cursor
    fprintf(stdout,"\033[%u;%uH", kitty_context->target_row, kitty_context->target_column);

    for (size_t i = 0; i < output_length; i += CHUNK_SIZE) {

        char* chunk = kitty_context->encoded_data + i;

        size_t remaining = output_length - i;

        size_t chunk_length = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        int more_chunks = remaining > CHUNK_SIZE ? 1 : 0;

        if (i == 0) {
            fprintf(
                stdout,
                "\x1b_Ga=t,"
                "i=%u,"
                "f=%d,"
                "s=%u,"
                "v=%u,"
                "X=%u,"
                "Y=%u,"
                "c=%u,"
                "r=%u,"
                "C=%d,"
                "m=%d,"
                "q=1;",
                frame_id,
                kitty_context->kitty_format,
                frame->width,
                frame->height,
                kitty_context->offset_x,
                kitty_context->offset_y, 
                kitty_context->columns,
                kitty_context->rows,
                kitty_context->cursor_policy,
                more_chunks
            );
        }
        else {
            fprintf(
                stdout,
                "\x1b_Gm=%d,q=1;",
                more_chunks
            );
        }

        fwrite(chunk, 1, chunk_length, stdout);
        fprintf(stdout, "\x1b\\");
    }
    fflush(stdout);

    return TPXL_OK;
}

TpxlResult tpxl_kitty_display(TpxlKittyContext* kitty_context, uint32_t frame_id) {
    
    fprintf(stdout,"\033[%u;%uH", kitty_context->target_row, kitty_context->target_column);

    fprintf(
        stdout,
        "\x1b_Ga=p,"
        "i=%u,"
        "X=%u,"
        "Y=%u,"
        "c=%u,"
        "r=%u,"
        "C=%d,"
        "q=1;",
        frame_id,
        kitty_context->offset_x,
        kitty_context->offset_y, 
        kitty_context->columns,
        kitty_context->rows,
        kitty_context->cursor_policy
    );

    fprintf(stdout, "\x1b\\");
    fflush(stdout);

    return TPXL_OK;
}

void tpxl_destroy_kitty_context(TpxlKittyContext* kitty_context) {

    if (!kitty_context) {
        return;
    }

    free(kitty_context->encoded_data);
    kitty_context->encoded_data = NULL;
    kitty_context = NULL;
}
