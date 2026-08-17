#include "tpxl/context.h"
#include "tpxl/terminal.h"
#include "tpxl/type.h"
#include "tpxl/viewport.h"

TpxlResult tpxl_init_context(TpxlContext* context) {

    if (!context) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->backend = TPXL_BACKEND_AUTO;
    context->scale_mode = TPXL_SCALE_FIT;
    context->horizontal_alignment = TPXL_ALIGN_START;
    context->vertical_alignment = TPXL_ALIGN_START;

    TpxlResult result = tpxl_init_terminal(&context->terminal);

    if (result != TPXL_OK) {
        return result;
    }

    result = tpxl_init_viewport(&context->viewport);

    if (result != TPXL_OK) {
        return result;
    }

    return TPXL_OK;
}

TpxlResult tpxl_context_set_scale_mode(TpxlContext* context, TpxlScaleMode scale_mode) {

    if (!context || scale_mode >= TPXL_SCALE_COUNT) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->scale_mode = scale_mode;

    return TPXL_OK;
}

TpxlResult tpxl_context_set_backend(TpxlContext* context, TpxlBackend backend) {

    if (!context || backend >= TPXL_BACKEND_COUNT) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->backend = backend;

    return TPXL_OK;
}

TpxlResult tpxl_context_set_alignment(TpxlContext* context, TpxlAlignment horizontal, TpxlAlignment vertical) {

    if (!context || horizontal >= TPXL_ALIGN_COUNT || vertical >= TPXL_ALIGN_COUNT) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->horizontal_alignment = horizontal;
    context->vertical_alignment = vertical;

    return TPXL_OK;
}

TpxlResult tpxl_set_context_viewport(TpxlContext* context, const TpxlViewport* viewport) {

    if (!context || !viewport) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->viewport = *viewport;

    return TPXL_OK;
}

TpxlResult tpxl_update_context_terminal(TpxlContext* context) {

    if (!context) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_query_terminal(&context->terminal);
}

TpxlResult tpxl_update_context_viewport(TpxlContext* context, uint32_t content_width, uint32_t content_height) {

    if (!context || !content_width || !content_height) {
        return TPXL_INVALID_ARGUMENT;
    }

    uint32_t area_width =  context->terminal.pixel_width;
    uint32_t area_height = context->terminal.pixel_height * context->terminal.cell_width / context->terminal.cell_height;

    TpxlResult result = TPXL_OK;

    switch (context->scale_mode) {

        case TPXL_SCALE_NONE:
            context->viewport.width = content_width;
            context->viewport.height = content_height;
            break;

        case TPXL_SCALE_FIT: 
            result = tpxl_viewport_fit(&context->viewport, area_width, area_height, content_width, content_height);
            break;

        default:
            result = TPXL_INVALID_ARGUMENT;
            break;
    }

    switch (context->horizontal_alignment) {

        case TPXL_ALIGN_START:
            context->viewport.x = 0;
            break;
        case TPXL_ALIGN_CENTER:
            context->viewport.x = (area_width - context->viewport.width) / 2;
            break;
        case TPXL_ALIGN_END:
            context->viewport.x = area_width - context->viewport.width;
            break;
        default:
            result = TPXL_INVALID_ARGUMENT;
            break;
    }

    switch (context->vertical_alignment) {

        case TPXL_ALIGN_START:
            context->viewport.y = 0;
            break;
        case TPXL_ALIGN_CENTER:
            context->viewport.y = (area_height - context->viewport.height) / 2;
            break;
        case TPXL_ALIGN_END:
            context->viewport.y = area_height - context->viewport.height;
            break;
        default:
            result = TPXL_INVALID_ARGUMENT;
            break;
    }

    return result;
}
