#ifndef TPXL_ITERM2_BACKEND_H
#define TPXL_ITERM2_BACKEND_H

#include <stdint.h>
#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/renderer.h"

#include "util/thread.h"

typedef struct {
    size_t frame_size;
    
    uint32_t output_width;
    uint32_t output_height;

    uint32_t output_columns;
    uint32_t output_rows;

    char* encoded_data;
    size_t encoded_capacity;

    uint32_t png_format;
    TpxlMediaType media_type;
    TpxlCursorPolicy cursor_policy;

    pthread_mutex_t output_mutex;
    bool output_mutex_initialized;
} TpxlIterm2Context;

TpxlResult tpxl_iterm2_context_init(TpxlIterm2Context* iterm2_context);
TpxlResult tpxl_iterm2_set_frame(TpxlIterm2Context* iterm2_context, const TpxlTerminal* terminal, uint32_t width, uint32_t height, TpxlFormat format);
TpxlResult tpxl_iterm2_set_media_policy(TpxlIterm2Context* iterm2_context, TpxlMediaType media_type);

TpxlResult tpxl_iterm2_direct_render(TpxlIterm2Context* iterm2_context, const TpxlImage* frame, uint32_t row, uint32_t column);
void tpxl_iterm2_context_destroy(TpxlIterm2Context* iterm2_context);

#endif
