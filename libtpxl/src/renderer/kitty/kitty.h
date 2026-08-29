#ifndef TPXL_RENDERER_KITTY_H
#define TPXL_RENDERER_KITTY_H

#include <stdint.h>
#include <stdio.h>

#include "tpxl/context.h"
#include "tpxl/type.h"

typedef struct {
    size_t frame_size;

    char* encoded_data;
    size_t encoded_capacity;

    uint8_t* compressed_data;
    size_t compressed_capacity;

    int kitty_format;
    int cursor_policy;

    uint32_t columns;
    uint32_t rows;
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t cell_x;
    uint32_t cell_y;
    uint32_t target_column;
    uint32_t target_row;

} TpxlKittyContext;

TpxlResult tpxl_set_kitty_context(TpxlKittyContext* kitty_context, TpxlContext* context);
TpxlResult tpxl_set_kitty_frame(TpxlKittyContext* kitty_context, uint32_t width, uint32_t height, TpxlFormat format);
TpxlResult tpxl_set_kitty_cursor_policy(TpxlKittyContext* kitty_context, TpxlMediaType media_type);

TpxlResult tpxl_kitty_render(TpxlKittyContext* kitty_context, TpxlImage* frame);
TpxlResult tpxl_kitty_transmit(TpxlKittyContext* kitty_context, TpxlImage* frame, uint32_t frame_id);
TpxlResult tpxl_kitty_display(TpxlKittyContext* kitty_context, uint32_t frame_id);
void tpxl_kitty_delete(uint32_t frame_id);
void tpxl_destroy_kitty_context(TpxlKittyContext* kitty_context);

#endif
