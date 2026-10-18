#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>
#include <zlib.h>

#include "tpxl/type.h"

#include "iterm2_backend.h"
#include "renderer/image_map.h"
#include "util/base64.h"

typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} TpxlPngBuffer;

static void tpxl_png_write(png_structp png_ptr, png_bytep data, png_size_t size) {

    TpxlPngBuffer* buffer = png_get_io_ptr(png_ptr);

    if ((size_t) size > SIZE_MAX - buffer->size) {
        png_error(png_ptr, "PNG buffer size overflow");
    }

    size_t required = buffer->size + (size_t)size;

    if (required > buffer->capacity) {
        size_t capacity = buffer->capacity > 0 ? buffer->capacity : 4096;

        while (capacity < required) {
            if (capacity > SIZE_MAX / 2) {
                capacity = required;
                break;
            }

            capacity *= 2;
        }

        uint8_t* data = realloc(buffer->data, capacity);

        if (!data) {
            png_error(png_ptr, "Failed to grow PNG buffer");
        }

        buffer->data = data;
        buffer->capacity = capacity;
    }

    memcpy(buffer->data + buffer->size, data, size);
    buffer->size = required;
}

static void tpxl_png_flush(png_structp png_ptr) {
    (void)png_ptr;
}

static TpxlResult tpxl_encode_png(TpxlIterm2Context* iterm2_context, const TpxlImage* frame, uint8_t** png_data, size_t* png_size) {

    assert(iterm2_context && frame && png_data && png_size);

    *png_data = NULL;
    *png_size = 0;

    int color_type = 0;
    int channels = 0;

    switch (iterm2_context->png_format) {
        case PNG_FORMAT_RGB:
            color_type = PNG_COLOR_TYPE_RGB;
            channels = 3;
            break;

        case PNG_FORMAT_RGBA:
            color_type = PNG_COLOR_TYPE_RGBA;
            channels = 4;
            break;

        default:
            return TPXL_UNSUPPORTED_FORMAT;
    }

    if (frame->width == 0 || frame->height == 0 ||
        frame->width > SIZE_MAX / channels) {
        return TPXL_INVALID_ARGUMENT;
    }

    size_t row_size = (size_t)frame->width * channels;

    if (frame->height > SIZE_MAX / row_size) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlPngBuffer* buffer = calloc(1, sizeof(TpxlPngBuffer));

    if (!buffer) {
        return TPXL_OUT_OF_MEMORY;
    }

    png_structp png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        NULL,
        NULL,
        NULL
    );

    if (!png_ptr) {
        free(buffer);
        return TPXL_OUT_OF_MEMORY;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        free(buffer);
        return TPXL_OUT_OF_MEMORY;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(buffer->data);
        free(buffer);
        return TPXL_ENCODING_FAILED;
    }

    png_set_write_fn(png_ptr, buffer, tpxl_png_write, tpxl_png_flush);

    png_set_IHDR(
        png_ptr,
        info_ptr,
        frame->width,
        frame->height,
        8,
        color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    // Prioritize encoding speed over compression ratio.
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
    png_set_compression_level(png_ptr, Z_BEST_SPEED);

    png_write_info(png_ptr, info_ptr);

    for (uint32_t y = 0; y < frame->height; y++) {
        png_bytep row = frame->pixels + (size_t) y * row_size;
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    *png_data = buffer->data;
    *png_size = buffer->size;

    free(buffer);

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

    TpxlResult result = tpxl_encode_png(iterm2_context, frame, &png_data, &png_size);

    if (result != TPXL_OK) {
        return result;
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

    TpxlResult result = tpxl_encode_png(iterm2_context, frame, &png_data, &png_size);
    
    if (result != TPXL_OK) {
        return result;
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

    result = tpxl_image_map_insert(&iterm2_context->image_map, &entry, frame_id);

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