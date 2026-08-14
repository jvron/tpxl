#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kitty.h"
#include "tpxl/type.h"
#include "util/base64.h"

TpxlResult tpxl_init_kitty_context(TpxlKittyContext* kitty_context, TpxlContext* context, TpxlImage* frame, TpxlMediaType media_type) {

    if (!kitty_context || !context || !frame ){
        return TPXL_INVALID_ARGUMENT;
    }

    kitty_context->encoded_data = NULL;

    int channels = tpxl_format_to_channels(frame->format);
    kitty_context->frame_length = frame->width * frame->height * channels;

    kitty_context->encoded_length = tpxl_base64_encoded_size(kitty_context->frame_length);

    char* data = malloc(kitty_context->encoded_length);

    if (!data) {
        return TPXL_OUT_OF_MEMORY;
    }

    kitty_context->encoded_data = data;

    switch (frame->format) {

        case TPXL_FORMAT_RGB:
            kitty_context->kitty_format = 24;
            break;
        case TPXL_FORMAT_RGBA:
            kitty_context->kitty_format = 32;
            break;
        default:
            free(data);
            return TPXL_UNSUPPORTED_FORMAT;
    }

    switch (media_type) {
        case TPXL_MEDIA_STILL:
            kitty_context->cursor_policy = 0;
            break;
        case TPXL_MEDIA_ANIMATED:
            kitty_context->cursor_policy = 1;
            break;
        default:
            free(data);
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

TpxlResult tpxl_kitty_render(TpxlKittyContext* kitty_context, TpxlImage* frame) {

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    size_t output_length = 0;

    if (tpxl_base64_encode(frame->pixels, kitty_context->frame_length, kitty_context->encoded_data, &output_length) != TPXL_OK) {
        return TPXL_ENCODING_FAILED;
    }

    // move cursor
    fprintf(stdout,"\033[%u;%uH", kitty_context->target_row, kitty_context->target_column);

    const size_t CHUNK_SIZE = 4096;

    for (size_t i = 0; i < output_length; i += CHUNK_SIZE) {

        char* chunk = kitty_context->encoded_data + i;

        size_t remaining = output_length - i;

        size_t chunk_length = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        int more_chunks = remaining > CHUNK_SIZE ? 1 : 0;

        fprintf(
            stdout,
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
    if (tpxl_base64_encode(frame->pixels, kitty_context->frame_length, kitty_context->encoded_data, &output_length) != TPXL_OK) {
        return TPXL_ENCODING_FAILED;
    }

    // move cursor
    fprintf(stdout,"\033[%u;%uH", kitty_context->target_row, kitty_context->target_column);

    const size_t CHUNK_SIZE = 4096;

    for (size_t i = 0; i < output_length; i += CHUNK_SIZE) {

        char* chunk = kitty_context->encoded_data + i;

        size_t remaining = output_length - i;

        size_t chunk_length = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        int more_chunks = remaining > CHUNK_SIZE ? 1 : 0;

        fprintf(
            stdout,
            "\x1b_Ga=t,"
            "i=%d,"
            "f=%d,"
            "s=%d,"
            "v=%d,"
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

        fwrite(chunk, 1, chunk_length, stdout);
        fprintf(stdout, "\x1b\\");
    }
    fflush(stdout);

    return TPXL_OK;
}

TpxlResult tpxl_kitty_display(TpxlKittyContext* kitty_context, TpxlImage* frame, uint32_t frame_id) {
    
    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    fprintf(stdout,"\033[%u;%uH", kitty_context->target_row, kitty_context->target_column);

    fprintf(
        stdout,
        "\x1b_Ga=p,"
        "i=%d,"
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

    memset(kitty_context, 0, sizeof(TpxlKittyContext));
}
