#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sixel.h>

#include "tpxl/image.h"
#include "tpxl/renderer.h"
#include "tpxl/type.h"

#include "sixel_backend.h"
#include "sixel_image_map.h"

static int tpxl_write_stdout(char* data, int chunk_size, void* priv) {

    (void)priv;

    if (fwrite(data, 1, chunk_size, stdout) == chunk_size) {
        return 0;
    }

    return -1;
}

static int tpxl_write_image_map(char* data, int chunk_size, void* priv) {

    TpxlSixelContext* sixel_context = priv; 

    if (!sixel_context->encoded_buffer) {

        char* buffer = malloc(sixel_context->encoded_buffer_capacity);

        if (!buffer) {
            return -1;
        }

        sixel_context->encoded_buffer = buffer;
    }

    size_t required = sixel_context->encoded_buffer_size + chunk_size;
    
    if ( required > sixel_context->encoded_buffer_capacity) {

        size_t new_capacity = sixel_context->encoded_buffer_capacity;

        while (new_capacity < required) {
            new_capacity *= 2;
        }

        char* encoded_buffer = realloc(sixel_context->encoded_buffer, new_capacity);

        if (!encoded_buffer) {
            return -1;
        }

        sixel_context->encoded_buffer = encoded_buffer;
        sixel_context->encoded_buffer_capacity = new_capacity;
    }

    memcpy(sixel_context->encoded_buffer + sixel_context->encoded_buffer_size, data, chunk_size);
    sixel_context->encoded_buffer_size += chunk_size;

    return 0;
}

TpxlResult tpxl_init_sixel_context(TpxlSixelContext* sixel_context) {

    assert(sixel_context);

    size_t initial_size = 16382;
    char* encoded_buffer = malloc(initial_size);

    if (!encoded_buffer) {
        return TPXL_OUT_OF_MEMORY;
    }

    sixel_context->encoded_buffer_size = 0;
    sixel_context->encoded_buffer_capacity = initial_size;
    sixel_context->encoded_buffer = encoded_buffer;

    if (tpxl_init_sixel_image_map(&sixel_context->image_map) != TPXL_OK) {
        return TPXL_SIXEL_BACKEND_CREATION_FAILED;
    }

    if (sixel_output_new(&sixel_context->output_stdout, tpxl_write_stdout, NULL, NULL) != SIXEL_OK) {
        return TPXL_SIXEL_BACKEND_CREATION_FAILED;
    }

    if (sixel_output_new(&sixel_context->output_image_map, tpxl_write_image_map, sixel_context, NULL) != SIXEL_OK) {
        sixel_output_unref(sixel_context->output_stdout);
        return TPXL_SIXEL_BACKEND_CREATION_FAILED;
    }

    if (sixel_dither_new(&sixel_context->dither, 256, NULL) != SIXEL_OK) {
        sixel_output_unref(sixel_context->output_image_map);
        sixel_output_unref(sixel_context->output_stdout);
        return TPXL_SIXEL_BACKEND_CREATION_FAILED;
    }

    return TPXL_OK;
}

TpxlResult tpxl_set_sixel_frame(TpxlSixelContext* sixel_context, const TpxlTerminal* terminal, uint32_t width, uint32_t height, TpxlFormat format) {

    assert(sixel_context && terminal);

    switch (format) {
        case TPXL_FORMAT_RGB:
            sixel_dither_set_pixelformat(sixel_context->dither, SIXEL_PIXELFORMAT_RGB888);
            sixel_context->sixel_format = SIXEL_PIXELFORMAT_RGB888;
            break;
        case TPXL_FORMAT_RGBA:
            sixel_dither_set_pixelformat(sixel_context->dither, SIXEL_PIXELFORMAT_RGBA8888);
            sixel_context->sixel_format = SIXEL_PIXELFORMAT_RGBA8888;
            break;
        default:
            return TPXL_UNSUPPORTED_FORMAT;
    }

    // convert pixel dimensions to terminal cells
    sixel_context->columns = (width + terminal->cell_width - 1) / terminal->cell_width;
    sixel_context->rows = (height + terminal->cell_height - 1) / terminal->cell_height;
    
    sixel_context->output_width = width;
    sixel_context->output_height = height;

    sixel_context->frame_size = width * height * tpxl_format_to_channels(format);

    return TPXL_OK;
}

TpxlResult tpxl_set_sixel_media_policy(TpxlSixelContext* sixel_context, TpxlMediaType media_type) {

    if (!sixel_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (media_type) {
        case TPXL_MEDIA_IMAGE:
            sixel_dither_set_diffusion_type(sixel_context->dither, SIXEL_DIFFUSE_STUCKI);
            sixel_context->cursor_policy = TPXL_CURSOR_ADVANCE;
            break;
        case TPXL_MEDIA_ANIMATION:
        case TPXL_MEDIA_VIDEO:
            sixel_dither_set_diffusion_type(sixel_context->dither, SIXEL_DIFFUSE_FS);
            sixel_context->cursor_policy = TPXL_CURSOR_PRESERVE;
            break;
        default:
            return TPXL_INVALID_ARGUMENT;
    }

    sixel_context->media_type = media_type;

    return TPXL_OK;
}

TpxlResult tpxl_sixel_direct_render(TpxlSixelContext* sixel_context, TpxlImage* frame, uint32_t row, uint32_t column) {

    assert(sixel_context && frame && frame->pixels);

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    if (sixel_context->media_type == TPXL_MEDIA_IMAGE) {
        if (tpxl_resize_image(frame, sixel_context->output_width, sixel_context->output_height) != TPXL_OK) {
            return TPXL_IMAGE_RESIZE_FAILED;
        }
    }

    int ret = sixel_dither_initialize(
        sixel_context->dither,
        frame->pixels,
        frame->width,
        frame->height,
        sixel_context->sixel_format,
        SIXEL_LARGE_LUM,
        SIXEL_REP_AVERAGE_PIXELS,
        SIXEL_QUALITY_FULL
    );

    if (ret != SIXEL_OK) {
        return TPXL_RENDER_FAILED;
    }

   fprintf(stdout, "\033[%u;%uH", row, column);

    ret = sixel_encode(
        frame->pixels,
        frame->width, 
        frame->height,
        0, 
        sixel_context->dither, 
        sixel_context->output_stdout
    );

    if (ret != SIXEL_OK) {
        return TPXL_RENDER_FAILED;
    }

    // Sixel advances the cursor by default
    if (sixel_context->cursor_policy == TPXL_CURSOR_PRESERVE) {
        fprintf(stdout, "\033[%u;%uH", row, column);
    }

    fflush(stdout);

    return TPXL_OK;
}

TpxlResult tpxl_sixel_upload(TpxlSixelContext* sixel_context, TpxlImage* frame, uint32_t frame_id) {

    assert(sixel_context && frame && frame->pixels);

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    if (sixel_context->media_type == TPXL_MEDIA_IMAGE) {
        if (tpxl_resize_image(frame, sixel_context->output_width, sixel_context->output_height) != TPXL_OK) {
            return TPXL_IMAGE_RESIZE_FAILED;
        }
    }

    sixel_dither_unref(sixel_context->dither);
    sixel_dither_new(&sixel_context->dither, 128, NULL);

    sixel_dither_set_pixelformat(sixel_context->dither, sixel_context->sixel_format);
    sixel_dither_set_diffusion_type(sixel_context->dither, SIXEL_DIFFUSE_FS);

    int ret = sixel_dither_initialize(
        sixel_context->dither,
        frame->pixels,
        frame->width,
        frame->height,
        sixel_context->sixel_format,
        SIXEL_LARGE_LUM,
        SIXEL_REP_AVERAGE_PIXELS,
        SIXEL_QUALITY_HIGH
    );

    if (ret != SIXEL_OK) {
        return TPXL_RENDER_FAILED;
    }

    ret = sixel_encode(
        frame->pixels,
        frame->width, 
        frame->height,
        0, 
        sixel_context->dither, 
        sixel_context->output_image_map
    );

    if (ret != SIXEL_OK) {
        return TPXL_ENCODING_FAILED;
    }

    TpxlSixelImage image = {
        .data = sixel_context->encoded_buffer,
        .size = sixel_context->encoded_buffer_size,
        .id = frame_id,
        .row = 0,
        .column = 0,
        .rows = sixel_context->rows,
        .columns = sixel_context->columns,
    };
    
    TpxlResult result = tpxl_sixel_image_map_insert(&sixel_context->image_map, &image, frame_id);

    if (result != TPXL_OK) {
        free(sixel_context->encoded_buffer);
        sixel_context->encoded_buffer = NULL;
        sixel_context->encoded_buffer_size = 0;
        return result;
    }

    sixel_context->encoded_buffer = NULL;
    sixel_context->encoded_buffer_size = 0;

    return TPXL_OK;
}

TpxlResult tpxl_sixel_display(TpxlSixelContext* sixel_context, uint32_t frame_id, uint32_t row, uint32_t column) {

    assert(sixel_context);

    TpxlSixelImage* sixel_image = NULL;
    TpxlResult result = tpxl_get_sixel_image(&sixel_context->image_map, frame_id,  &sixel_image);

    if (result != TPXL_OK) {
        return result;
    }

    fprintf(stdout, "\033[%u;%uH", row, column);

    if (fwrite(sixel_image->data, 1, sixel_image->size, stdout) != sixel_image->size) {
        return TPXL_RENDER_FAILED;
    }

    sixel_image->row = row;
    sixel_image->column = column;

    if (sixel_context->cursor_policy == TPXL_CURSOR_PRESERVE) {
        fprintf(stdout, "\033[%u;%uH", row, column);
    }

    fflush(stdout);

    return TPXL_OK;
}

void tpxl_sixel_delete_placement(TpxlSixelContext* sixel_context, uint32_t frame_id) {

    assert(sixel_context);

    TpxlSixelImage* sixel_image = NULL;
    if (tpxl_get_sixel_image(&sixel_context->image_map, frame_id, &sixel_image) != TPXL_OK) {
        return;
    }

    fprintf(stdout, "\033[%u;%uH", sixel_image->row, sixel_image->column);

    for (uint32_t i = 0; i < sixel_image->rows; i++) {

        for (uint32_t j = 0; j < sixel_image->columns; j++) {
            fprintf(stdout, " ");
        }

        fprintf(stdout, "\033[%u;%uH", sixel_image->row + i + 1, sixel_image->column);
    }

    fprintf(stdout, "\033[%u;%uH", sixel_image->row, sixel_image->column);

    fflush(stdout);
}

void tpxl_sixel_delete_data(TpxlSixelContext* sixel_context, uint32_t frame_id) {

    assert(sixel_context);

    tpxl_sixel_image_map_remove(&sixel_context->image_map, frame_id);
}

void tpxl_destroy_sixel_context(TpxlSixelContext* sixel_context) {

    if (!sixel_context) {
        return;
    }

    tpxl_destroy_sixel_image_map(&sixel_context->image_map);

    free(sixel_context->encoded_buffer);
    sixel_context->encoded_buffer = NULL;

    sixel_dither_unref(sixel_context->dither);
    sixel_context->dither = NULL;

    sixel_output_unref(sixel_context->output_image_map);
    sixel_context->output_image_map = NULL;

    sixel_output_unref(sixel_context->output_stdout);
    sixel_context->output_stdout = NULL;
}