#ifndef TPXL_RENDERER_SIXEL_H
#define TPXL_RENDERER_SIXEL_H

#include <stdint.h>
#include <stdio.h>

#include <sixel.h>

#include "tpxl/context.h"
#include "tpxl/type.h"

typedef struct {
    size_t frame_size;

    uint32_t columns;
    uint32_t rows;
    uint32_t cell_x;
    uint32_t cell_y;
    uint32_t target_column;
    uint32_t target_row;

    sixel_output_t* output;
    sixel_dither_t* dither;
    int sixel_format;

} TpxlSixelContext;

TpxlResult tpxl_set_sixel_context(TpxlSixelContext* sixel_context, TpxlContext* context, bool create_sixel_objects);
TpxlResult tpxl_set_sixel_frame(TpxlSixelContext* sixel_context, uint32_t width, uint32_t height, TpxlFormat format);

TpxlResult tpxl_sixel_render(TpxlSixelContext* sixel_context, TpxlImage* frame);
void tpxl_destroy_sixel_context(TpxlSixelContext* sixel_context);

#endif

