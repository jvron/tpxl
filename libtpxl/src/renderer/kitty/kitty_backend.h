#ifndef TPXL_KITTY_BACKEND_H
#define TPXL_KITTY_BACKEND_H

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#include "tpxl/context.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

typedef struct {
    size_t frame_size;

    char* encoded_data;
    size_t encoded_capacity;

    uint8_t* compressed_data;
    size_t compressed_capacity;

    int kitty_format;
    TpxlCursorPolicy cursor_policy;

    pthread_mutex_t output_mutex;
    bool output_mutex_initialized;

    uint32_t columns;
    uint32_t rows;
    uint32_t offset_x;
    uint32_t offset_y;
} TpxlKittyContext;

TpxlResult tpxl_set_kitty_context(TpxlKittyContext* kitty_context, TpxlContext* context);
TpxlResult tpxl_set_kitty_frame(TpxlKittyContext* kitty_context, uint32_t width, uint32_t height, TpxlFormat format);
TpxlResult tpxl_set_kitty_media_policy(TpxlKittyContext* kitty_context, TpxlMediaType media_type);

TpxlResult tpxl_kitty_direct_render(TpxlKittyContext* kitty_context, TpxlImage* frame, uint32_t row, uint32_t column);
TpxlResult tpxl_kitty_transmit(TpxlKittyContext* kitty_context, TpxlImage* frame, uint32_t frame_id);
TpxlResult tpxl_kitty_display(TpxlKittyContext* kitty_context, uint32_t frame_id, uint32_t row, uint32_t column);
void tpxl_kitty_delete_placement(TpxlKittyContext* kitty_context, uint32_t frame_id);
void tpxl_kitty_delete_data(TpxlKittyContext* kitty_context, uint32_t frame_id);
void tpxl_destroy_kitty_context(TpxlKittyContext* kitty_context);

#endif
