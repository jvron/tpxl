#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sixel.h>

#include "tpxl/renderer.h"
#include "tpxl/type.h"

#include "sixel_backend.h"

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

TpxlResult tpxl_sixel_context_init(TpxlSixelContext* sixel_context) {

    assert(sixel_context);

    size_t initial_size = 16382;
    char* encoded_buffer = malloc(initial_size);

    if (!encoded_buffer) {
        return TPXL_OUT_OF_MEMORY;
    }

    sixel_context->encoded_buffer_size = 0;
    sixel_context->encoded_buffer_capacity = initial_size;
    sixel_context->encoded_buffer = encoded_buffer;

    sixel_context->clear_buffer = NULL;
    sixel_context->clear_buffer_capacity = 0;

    if (tpxl_image_map_init(&sixel_context->image_map) != TPXL_OK) {
        free(encoded_buffer);
        return TPXL_BACKEND_CREATION_FAILED;
    }

    if (sixel_output_new(&sixel_context->output_stdout, tpxl_write_stdout, NULL, NULL) != SIXEL_OK) {
        free(encoded_buffer);
        return TPXL_BACKEND_CREATION_FAILED;
    }

    if (sixel_output_new(&sixel_context->output_image_map, tpxl_write_image_map, sixel_context, NULL) != SIXEL_OK) {
        free(encoded_buffer);
        sixel_output_unref(sixel_context->output_stdout);
        return TPXL_BACKEND_CREATION_FAILED;
    }

    sixel_context->video_dither = sixel_dither_get(BUILTIN_XTERM256);

    return TPXL_OK;
}

TpxlResult tpxl_sixel_set_frame(TpxlSixelContext* sixel_context, const TpxlTerminal* terminal, uint32_t width, uint32_t height, TpxlFormat format) {

    assert(sixel_context && terminal);

    switch (format) {
        case TPXL_FORMAT_RGB:
            sixel_context->sixel_format = SIXEL_PIXELFORMAT_RGB888;
            break;
        case TPXL_FORMAT_RGBA:
            sixel_context->sixel_format = SIXEL_PIXELFORMAT_RGBA8888;
            break;
        default:
            return TPXL_UNSUPPORTED_FORMAT;
    }

    if (sixel_context->media_type == TPXL_MEDIA_VIDEO) {
        sixel_dither_set_pixelformat(sixel_context->video_dither, sixel_context->sixel_format);
    }

    // convert pixel dimensions to terminal cells
    sixel_context->columns = (width + terminal->cell_width - 1) / terminal->cell_width;
    sixel_context->rows = (height + terminal->cell_height - 1) / terminal->cell_height;
    
    sixel_context->output_width = width;
    sixel_context->output_height = height;

    sixel_context->frame_size = width * height * tpxl_format_to_channels(format);

    return TPXL_OK;
}

TpxlResult tpxl_sixel_set_media_policy(TpxlSixelContext* sixel_context, TpxlMediaType media_type) {

    if (!sixel_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (media_type) {
        case TPXL_MEDIA_IMAGE:
            sixel_context->sixel_diffuse = SIXEL_DIFFUSE_STUCKI;
            sixel_context->sixel_quality = SIXEL_QUALITY_FULL;
            sixel_context->cursor_policy = TPXL_CURSOR_ADVANCE;
            break;
        case TPXL_MEDIA_ANIMATION:
            sixel_context->sixel_diffuse = SIXEL_DIFFUSE_FS;
            sixel_context->sixel_quality = SIXEL_QUALITY_HIGH;
            sixel_context->cursor_policy = TPXL_CURSOR_PRESERVE;
            break;
        case TPXL_MEDIA_VIDEO:
            sixel_context->sixel_diffuse = SIXEL_DIFFUSE_FS;
            sixel_dither_set_diffusion_type(sixel_context->video_dither, SIXEL_DIFFUSE_FS);
            sixel_context->sixel_quality = SIXEL_QUALITY_HIGH;
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

    int ret = 0;
    TpxlResult result = TPXL_OK;

    sixel_dither_t* dither = NULL;

    if (sixel_context->media_type != TPXL_MEDIA_VIDEO) {
        
        ret = sixel_dither_new(&dither, 256, NULL);

        if (ret != SIXEL_OK) {
            result = TPXL_RENDER_FAILED;
            goto cleanup;
        }

        sixel_dither_set_pixelformat(dither, sixel_context->sixel_format);
        sixel_dither_set_diffusion_type(dither, sixel_context->sixel_diffuse);

        ret = sixel_dither_initialize(
            dither,
            frame->pixels,
            frame->width,
            frame->height,
            sixel_context->sixel_format,
            SIXEL_LARGE_LUM,
            SIXEL_REP_AVERAGE_PIXELS,
            sixel_context->sixel_quality
        );

        if (ret != SIXEL_OK) {
            result = TPXL_RENDER_FAILED;
            goto cleanup;
        }
    } else {
        dither = sixel_context->video_dither;
    }

    fprintf(stdout, "\033[%u;%uH", row, column);

    ret = sixel_encode(
        frame->pixels,
        frame->width, 
        frame->height,
        0, 
        dither, 
        sixel_context->output_stdout
    );

    if (ret != SIXEL_OK) {
        result = TPXL_RENDER_FAILED;
        goto cleanup;
    }

    // Sixel advances the cursor by default
    if (sixel_context->cursor_policy == TPXL_CURSOR_PRESERVE) {
        fprintf(stdout, "\033[%u;%uH", row, column);
    }

    fflush(stdout);


cleanup:
    if (sixel_context->media_type != TPXL_MEDIA_VIDEO) {
        sixel_dither_destroy(dither);
    }

    return result;
}

TpxlResult tpxl_sixel_upload(TpxlSixelContext* sixel_context, TpxlImage* frame, uint32_t frame_id) {

    assert(sixel_context && frame && frame->pixels);

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    int ret = 0;
    TpxlResult result = TPXL_OK;

    sixel_dither_t* dither = NULL;

    if (sixel_context->media_type != TPXL_MEDIA_VIDEO) {
        
        ret = sixel_dither_new(&dither, 256, NULL);

        if (ret != SIXEL_OK) {
            result = TPXL_RENDER_FAILED;
            goto cleanup;
        }

        sixel_dither_set_pixelformat(dither, sixel_context->sixel_format);
        sixel_dither_set_diffusion_type(dither, sixel_context->sixel_diffuse);

        ret = sixel_dither_initialize(
            dither,
            frame->pixels,
            frame->width,
            frame->height,
            sixel_context->sixel_format,
            SIXEL_LARGE_LUM,
            SIXEL_REP_AVERAGE_PIXELS,
            sixel_context->sixel_quality
        );

        if (ret != SIXEL_OK) {
            result = TPXL_RENDER_FAILED;
            goto cleanup;
        }
    } else {
        dither = sixel_context->video_dither;
    }

    ret = sixel_encode(
        frame->pixels,
        frame->width, 
        frame->height,
        0, 
        dither, 
        sixel_context->output_image_map
    );

    if (ret != SIXEL_OK) {
        result = TPXL_ENCODING_FAILED;
        goto cleanup;
    }

    TpxlImageMapEntry entry = {
        .data = sixel_context->encoded_buffer,
        .size = sixel_context->encoded_buffer_size,
        .id = frame_id,
        .row = 0,
        .column = 0,
        .rows = sixel_context->rows,
        .columns = sixel_context->columns,
    };
    
    result = tpxl_image_map_insert(&sixel_context->image_map, &entry, frame_id);

    if (result != TPXL_OK) {
        free(sixel_context->encoded_buffer);
    }

    sixel_context->encoded_buffer = NULL;
    sixel_context->encoded_buffer_size = 0;

cleanup:
    if (sixel_context->media_type != TPXL_MEDIA_VIDEO) {
        sixel_dither_destroy(dither);
    }

    return result;
}

TpxlResult tpxl_sixel_display(TpxlSixelContext* sixel_context, uint32_t frame_id, uint32_t row, uint32_t column) {

    assert(sixel_context);

    TpxlImageMapEntry* entry = NULL;
    TpxlResult result = tpxl_image_map_get(&sixel_context->image_map, frame_id,  &entry);

    if (result != TPXL_OK) {
        return result;
    }

    fprintf(stdout, "\033[%u;%uH", row, column);

    if (fwrite(entry->data, 1, entry->size, stdout) != entry->size) {
        return TPXL_RENDER_FAILED;
    }

    entry->row = row;
    entry->column = column;

    if (sixel_context->cursor_policy == TPXL_CURSOR_PRESERVE) {
        fprintf(stdout, "\033[%u;%uH", row, column);
    }

    fflush(stdout);
    
    return TPXL_OK;
}

void tpxl_sixel_delete_placement(TpxlSixelContext* sixel_context, uint32_t frame_id) {

    assert(sixel_context);

    TpxlImageMapEntry* entry = NULL;
    if (tpxl_image_map_get(&sixel_context->image_map, frame_id, &entry) != TPXL_OK) {
        return;
    }

    if (entry->columns > sixel_context->clear_buffer_capacity) {

        size_t capacity = sixel_context->clear_buffer_capacity;

        if (capacity == 0) {
            capacity = 64;
        }

        while (capacity < entry->columns) {
            capacity *= 2;
        }

        char* buffer = realloc(sixel_context->clear_buffer, capacity);

        if (!buffer) {
            return;
        }

        sixel_context->clear_buffer = buffer;
        sixel_context->clear_buffer_capacity = capacity;
    }

    memset(sixel_context->clear_buffer, ' ', entry->columns);

    fprintf(stdout, "\033[%u;%uH", entry->row, entry->column);

    for (uint32_t i = 0; i < entry->rows; i++) {

        fwrite(sixel_context->clear_buffer, 1, entry->columns, stdout);

        fprintf(stdout, "\033[%u;%uH", entry->row + i + 1, entry->column);
    }

    fprintf(stdout, "\033[%u;%uH", entry->row, entry->column);

    fflush(stdout);
}

void tpxl_sixel_delete_data(TpxlSixelContext* sixel_context, uint32_t frame_id) {

    assert(sixel_context);

    tpxl_image_map_remove(&sixel_context->image_map, frame_id);
}

void tpxl_sixel_context_destroy(TpxlSixelContext* sixel_context) {

    if (!sixel_context) {
        return;
    }

    tpxl_image_map_destroy(&sixel_context->image_map);

    free(sixel_context->encoded_buffer);
    sixel_context->encoded_buffer = NULL;

    free(sixel_context->clear_buffer);
    sixel_context->clear_buffer = NULL;

    sixel_dither_unref(sixel_context->video_dither);
    sixel_context->video_dither = NULL;

    sixel_output_unref(sixel_context->output_image_map);
    sixel_context->output_image_map = NULL;

    sixel_output_unref(sixel_context->output_stdout);
    sixel_context->output_stdout = NULL;
}