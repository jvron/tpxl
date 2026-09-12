#include <stdio.h>

#include <sixel.h>

#include "tpxl/context.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

#include "sixel_backend.h"

static int tpxl_write_stdout(char* data, int size, void* priv) {

    (void)priv;

    if (fwrite(data, 1, size, stdout) == size) {
        return 0;
    }

    return -1;
}

TpxlResult tpxl_set_sixel_context(TpxlSixelContext* sixel_context, TpxlContext* context, bool create_sixel_objects) {

    if (!sixel_context || !context){
        return TPXL_INVALID_ARGUMENT;
    }

    // convert viewport dimensions to terminal cells
    sixel_context->columns = (context->viewport.width + context->terminal.cell_width - 1) / context->terminal.cell_width;
    sixel_context->rows = (context->viewport.height + context->terminal.cell_height - 1) / context->terminal.cell_height;

    // convert viewport x and y to cells
    sixel_context->cell_x = context->viewport.x / context->terminal.cell_width;
    sixel_context->cell_y = context->viewport.y / context->terminal.cell_height;

    sixel_context->target_column = context->terminal.cursor_column + sixel_context->cell_x;
    sixel_context->target_row = context->terminal.cursor_row + sixel_context->cell_y;

    // set output dimensions to the viewport's pixel dimensions for image scaling
    sixel_context->output_width = context->viewport.width;
    sixel_context->output_height = context->viewport.height;

    if (!create_sixel_objects) {
        return TPXL_OK;
    }

    if (sixel_output_new(&sixel_context->output, tpxl_write_stdout, NULL, NULL) != SIXEL_OK) {
        return TPXL_SIXEL_BACKEND_CREATION_FAILED;
    }

    if (sixel_dither_new(&sixel_context->dither, 256, NULL) != SIXEL_OK) {
        sixel_output_unref(sixel_context->output);
        return TPXL_SIXEL_BACKEND_CREATION_FAILED;
    }

    return TPXL_OK;
}

TpxlResult tpxl_set_sixel_frame(TpxlSixelContext* sixel_context, uint32_t width, uint32_t height, TpxlFormat format) {

    if (!sixel_context) {
        return TPXL_INVALID_ARGUMENT;
    }

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

    sixel_dither_set_diffusion_type(sixel_context->dither, SIXEL_DIFFUSE_STUCKI);
    
    sixel_context->frame_size = width * height * tpxl_format_to_channels(format);

    return TPXL_OK;
}

TpxlResult tpxl_set_sixel_media_policy(TpxlSixelContext* sixel_context, TpxlMediaType media_type) {

    if (!sixel_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (media_type == TPXL_MEDIA_UNKNOWN || media_type == TPXL_MEDIA_AUDIO) {
        return TPXL_INVALID_ARGUMENT;
    }

    sixel_context->media_type = media_type;

    return TPXL_OK;
}

TpxlResult tpxl_sixel_render(TpxlSixelContext* sixel_context, TpxlImage* frame) {

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

    fprintf(stdout,"\033[%u;%uH", sixel_context->target_row, sixel_context->target_column);

    ret = sixel_encode(
        frame->pixels,
        frame->width, 
        frame->height,
        0, 
        sixel_context->dither, 
        sixel_context->output
    );

    if (ret != SIXEL_OK) {
        return TPXL_RENDER_FAILED;
    }

    return TPXL_OK;
}

void tpxl_destroy_sixel_context(TpxlSixelContext* sixel_context) {

    if (!sixel_context) {
        return;
    }

    sixel_dither_unref(sixel_context->dither);
    sixel_context->dither = NULL;

    sixel_output_unref(sixel_context->output);
    sixel_context->output = NULL;
}