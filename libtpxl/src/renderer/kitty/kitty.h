#ifndef TPXL_RENDERER_KITTY_H
#define TPXL_RENDERER_KITTY_H

#include <stdint.h>
#include <stdio.h>

#include "tpxl/context.h"
#include "tpxl/type.h"

typedef struct {
    size_t frame_length;

    char* encoded_data;
    size_t encoded_length;

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

TpxlResult tpxl_init_kitty_context(TpxlKittyContext* kitty_context, TpxlContext* context, TpxlImage* frame, TpxlMediaType media_type); 
TpxlResult tpxl_kitty_render(TpxlKittyContext* kitty_context, TpxlImage* frame);
TpxlResult tpxl_kitty_transmit(TpxlKittyContext* kitty_context, TpxlImage* frame, uint32_t frame_id);
TpxlResult tpxl_kitty_display(TpxlKittyContext* kitty_context, uint32_t frame_id);
void tpxl_destroy_kitty_context(TpxlKittyContext* kitty_context);

#endif
