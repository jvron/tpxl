#ifndef TPXL_SIXEL_BACKEND_H
#define TPXL_SIXEL_BACKEND_H

#include <stdint.h>
#include <stdio.h>

#include <sixel.h>

#include "tpxl/context.h"
#include "tpxl/renderer.h"
#include "tpxl/type.h"

#include "sixel_image_map.h"

typedef struct {
    size_t frame_size;

    TpxlMediaType media_type;
    TpxlCursorPolicy cursor_policy;

    // Output dimensions in pixels
    uint32_t output_width;
    uint32_t output_height;

    // Output dimensions in cells
    uint32_t columns;
    uint32_t rows;

    TpxlSixelImageMap image_map;
    
    char* encoded_buffer;
    size_t encoded_buffer_size;
    size_t encoded_buffer_capacity;

    sixel_output_t* output_stdout;
    sixel_output_t* output_image_map;
    sixel_dither_t* dither;
    int sixel_format;
} TpxlSixelContext;

TpxlResult tpxl_set_sixel_context(TpxlSixelContext* sixel_context, TpxlContext* context, bool create_sixel_objects);
TpxlResult tpxl_set_sixel_frame(TpxlSixelContext* sixel_context, uint32_t width, uint32_t height, TpxlFormat format);
TpxlResult tpxl_set_sixel_media_policy(TpxlSixelContext* sixel_context, TpxlMediaType media_type);

TpxlResult tpxl_sixel_direct_render(TpxlSixelContext* sixel_context, TpxlImage* frame, uint32_t row, uint32_t column);
TpxlResult tpxl_sixel_upload(TpxlSixelContext* sixel_context, TpxlImage* frame, uint32_t frame_id);
TpxlResult tpxl_sixel_display(TpxlSixelContext* sixel_context, uint32_t frame_id, uint32_t row, uint32_t column);
void tpxl_sixel_delete_placement(TpxlSixelContext* sixel_context, uint32_t frame_id);
void tpxl_sixel_delete_data(TpxlSixelContext* sixel_context, uint32_t frame_id);
void tpxl_destroy_sixel_context(TpxlSixelContext* sixel_context);

#endif

