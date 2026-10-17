#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "tpxl/type.h"

#include "iterm2_backend.h"
#include "renderer/image_map.h"
#include "util/base64.h"

static TpxlResult tpxl_encode_png(TpxlIterm2Context* iterm2_context, const TpxlImage* frame, uint8_t** png_data, size_t* png_size) {

    assert(iterm2_context && frame && png_data);

    *png_data = NULL;
    *png_size = 0;

    png_image image;
    png_alloc_size_t buffer_size = 0;

    memset(&image, 0, sizeof(image));

    image.version = PNG_IMAGE_VERSION;
    image.width = frame->width;
    image.height = frame->height;
    image.format = iterm2_context->png_format;

    // Get buffer size
    if (!png_image_write_to_memory(&image, NULL, &buffer_size, 0, frame->pixels, 0, NULL)) {
        png_image_free(&image);
        return TPXL_ERROR;
    }

    uint8_t* buffer = malloc(buffer_size);

    if (!buffer) {
        png_image_free(&image);
        return TPXL_OUT_OF_MEMORY;
    }

    // Encode the PNG into memory.
    if (!png_image_write_to_memory(&image, buffer, &buffer_size, 0, frame->pixels, 0, NULL)) {
        free(buffer);
        png_image_free(&image);
        return TPXL_ERROR;
    }

    *png_data = buffer;
    *png_size = buffer_size;

    png_image_free(&image);

    return TPXL_OK;
}

TpxlResult tpxl_iterm2_context_init(TpxlIterm2Context* iterm2_context) {

    assert(iterm2_context);

    if (tpxl_image_map_init(&iterm2_context->image_map) != TPXL_OK) {
        return TPXL_BACKEND_CREATION_FAILED;
    }

    // output_mutex_initialized will be false initially
    if (!iterm2_context->output_mutex_initialized) {
        if (tpxl_mutex_init(&iterm2_context->output_mutex) != 0) {
            return TPXL_BACKEND_CREATION_FAILED;
        }
    
        iterm2_context->output_mutex_initialized = true;
    }

    return TPXL_OK;
}

TpxlResult tpxl_iterm2_set_frame(
    TpxlIterm2Context* iterm2_context,
    const TpxlTerminal* terminal, 
    uint32_t width, 
    uint32_t height, 
    TpxlFormat format
) 
{
    assert(iterm2_context && terminal);

    switch (format) {
        case TPXL_FORMAT_RGB:
            iterm2_context->png_format = PNG_FORMAT_RGB;
            break;
        case TPXL_FORMAT_RGBA:
            iterm2_context->png_format = PNG_FORMAT_RGBA;
            break;
        default:
            return TPXL_UNSUPPORTED_FORMAT;
    }

    // convert pixel dimensions to terminal cells
    iterm2_context->output_columns = (width + terminal->cell_width - 1) / terminal->cell_width;
    iterm2_context->output_rows = (height + terminal->cell_height - 1) / terminal->cell_height;
    
    iterm2_context->output_width = width;
    iterm2_context->output_height = height;

    iterm2_context->frame_size = (size_t) width * height * tpxl_format_to_channels(format);

    return TPXL_OK;
}

TpxlResult tpxl_iterm2_set_media_policy(TpxlIterm2Context* iterm2_context, TpxlMediaType media_type) {

    assert(iterm2_context);

    if (!iterm2_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (media_type) {
        case TPXL_MEDIA_IMAGE:
            iterm2_context->cursor_policy = TPXL_CURSOR_ADVANCE;
            break;
        case TPXL_MEDIA_ANIMATION:
        case TPXL_MEDIA_VIDEO:
            iterm2_context->cursor_policy = TPXL_CURSOR_PRESERVE;
            break;

        default:
            return TPXL_INVALID_ARGUMENT;
    }

    iterm2_context->media_type = media_type;

    return TPXL_OK;
}

TpxlResult tpxl_iterm2_direct_render(TpxlIterm2Context* iterm2_context, const TpxlImage* frame, uint32_t row, uint32_t column) {

    assert(iterm2_context && frame && frame->pixels);

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    uint8_t* png_data = NULL;
    size_t png_size = 0;

    if (tpxl_encode_png(iterm2_context, frame, &png_data, &png_size) != TPXL_OK) {
        return TPXL_RENDER_FAILED;
    }

    size_t encoded_size = tpxl_base64_encoded_size(png_size);

    if (encoded_size > iterm2_context->encoded_capacity) {

        char* encoded_buffer = realloc(iterm2_context->encoded_buffer, encoded_size);

        if (!encoded_buffer) {
            free(png_data);
            return TPXL_OUT_OF_MEMORY;
        }

        iterm2_context->encoded_buffer = encoded_buffer;
        iterm2_context->encoded_capacity = encoded_size;
    }

    size_t output_length = 0;
    if (tpxl_base64_encode(png_data, png_size, iterm2_context->encoded_buffer, &output_length) != TPXL_OK) {
        free(png_data);
        return TPXL_ENCODING_FAILED;
    }
        
    free(png_data);

    tpxl_mutex_lock(&iterm2_context->output_mutex);

    fprintf(stdout,"\033[%u;%uH", row, column);

    fprintf(
        stdout, 
        "\x1b]1337;File=inline=1;width=%d;height=%d;preserveAspectRatio=1:",
        iterm2_context->output_columns,
        iterm2_context->output_rows
    );

    if(fwrite(iterm2_context->encoded_buffer, 1, output_length, stdout) != output_length) {
        return TPXL_RENDER_FAILED;
    }

    fputc('\a', stdout);

    if (iterm2_context->cursor_policy == TPXL_CURSOR_PRESERVE) {
        fprintf(stdout, "\033[%u;%uH", row, column);
    }

    fflush(stdout);

    tpxl_mutex_unlock(&iterm2_context->output_mutex);

    return TPXL_OK;
}

TpxlResult tpxl_iterm2_upload(TpxlIterm2Context* iterm2_context, const TpxlImage* frame, uint32_t frame_id) {

    assert(iterm2_context && frame && frame->pixels);

    if (frame->format == TPXL_FORMAT_UNKNOWN) {
        return TPXL_INVALID_FORMAT;
    }

    uint8_t* png_data = NULL;
    size_t png_size = 0;

    if (tpxl_encode_png(iterm2_context, frame, &png_data, &png_size) != TPXL_OK) {
        return TPXL_RENDER_FAILED;
    }

    size_t encoded_size = tpxl_base64_encoded_size(png_size);

    if (encoded_size > iterm2_context->encoded_capacity) {

        char* encoded_buffer = realloc(iterm2_context->encoded_buffer, encoded_size);

        if (!encoded_buffer) {
            free(png_data);
            return TPXL_OUT_OF_MEMORY;
        }

        iterm2_context->encoded_buffer = encoded_buffer;
        iterm2_context->encoded_capacity = encoded_size;
    }

    size_t output_length = 0;
    if (tpxl_base64_encode(png_data, png_size, iterm2_context->encoded_buffer, &output_length) != TPXL_OK) {
        free(png_data);
        return TPXL_ENCODING_FAILED;
    }

    free(png_data);

    char* entry_data = malloc(output_length);

    if (!entry_data) {
        return TPXL_OUT_OF_MEMORY;
    }

    memcpy(entry_data, iterm2_context->encoded_buffer, output_length);

    TpxlImageMapEntry entry = {
        .data = entry_data,
        .size = output_length,
        .id = frame_id,
        .row = 0,
        .column = 0,
        .columns = iterm2_context->output_columns,
        .rows = iterm2_context->output_rows,
    };

    TpxlResult result = tpxl_image_map_insert(&iterm2_context->image_map, &entry, frame_id);

    if (result != TPXL_OK) {
        free(entry_data);
        return result;
    }

    return TPXL_OK;
}

TpxlResult tpxl_iterm2_display(TpxlIterm2Context* iterm2_context, uint32_t frame_id, uint32_t row, uint32_t column) {

    assert(iterm2_context);

    TpxlImageMapEntry* entry = NULL;
    TpxlResult result = tpxl_image_map_get(&iterm2_context->image_map, frame_id,  &entry);

    if (result != TPXL_OK) {
        return result;
    }

    tpxl_mutex_lock(&iterm2_context->output_mutex);

    fprintf(stdout, "\033[%u;%uH", row, column);

    fprintf(
        stdout, 
        "\x1b]1337;File=inline=1;width=%d;height=%d;preserveAspectRatio=1:",
        iterm2_context->output_columns,
        iterm2_context->output_rows
    );

    if (fwrite(entry->data, 1, entry->size, stdout) != entry->size) {
        return TPXL_RENDER_FAILED;
    }

    fputc('\a', stdout);

    if (iterm2_context->cursor_policy == TPXL_CURSOR_PRESERVE) {
        fprintf(stdout, "\033[%u;%uH", row, column);
    }

    fflush(stdout);

    tpxl_mutex_unlock(&iterm2_context->output_mutex);

    entry->row = row;
    entry->column = column;
    
    return TPXL_OK;
}

void tpxl_iterm2_context_destroy(TpxlIterm2Context* iterm2_context) {
    
    if (!iterm2_context) {
        return;
    }

    if (iterm2_context->output_mutex_initialized) {
        tpxl_mutex_destroy(&iterm2_context->output_mutex);
        iterm2_context->output_mutex_initialized = false;
    }

    tpxl_image_map_destroy(&iterm2_context->image_map);

    free(iterm2_context->encoded_buffer);
    iterm2_context->encoded_buffer = NULL;
}