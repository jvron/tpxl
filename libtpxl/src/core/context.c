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

    if (!context) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->scale_mode = scale_mode;

    return TPXL_OK;
}

TpxlResult tpxl_context_set_backend(TpxlContext* context, TpxlBackend backend) {

    if (!context) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->backend = backend;

    return TPXL_OK;
}

TpxlResult tpxl_set_viewport(TpxlContext* context, const TpxlViewport* viewport) {

    if (!context || !viewport) {
        return TPXL_INVALID_ARGUMENT;
    }

    context->viewport = *viewport;

    return TPXL_OK;
}

TpxlResult tpxl_update_context(TpxlContext* context) {

    if (!context) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_query_terminal(&context->terminal);
}

TpxlResult tpxl_update_viewport(TpxlContext* context, uint32_t content_width, uint32_t content_height) {

    if (!context || !content_width || !content_height) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = TPXL_OK;

    switch (context->scale_mode) {

        case TPXL_SCALE_NONE:
            context->viewport.x = 0;
            context->viewport.y = 0;
            context->viewport.width = content_width;
            context->viewport.height = content_height;
            break;

        case TPXL_SCALE_FIT: 
        {
            uint32_t area_width =  context->terminal.pixel_width;
            uint32_t area_height = context->terminal.pixel_height * context->terminal.cell_width / context->terminal.cell_height;
            result = tpxl_viewport_fit(&context->viewport, area_width, area_height, content_width, content_height);
            break;
        }

        default:
            break;
    }

    return result;
}
