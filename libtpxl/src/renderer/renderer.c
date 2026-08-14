#include <stdlib.h>

#include "kitty/kitty.h"
#include "tpxl/context.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

struct TpxlRendererImp {
    TpxlKittyContext kitty_context;
};

TpxlResult tpxl_create_renderer(TpxlRenderer** renderer, TpxlContext* context, TpxlImage* frame, TpxlMediaType media_type) {

    if (!renderer || !context || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    *renderer = malloc(sizeof(TpxlRenderer));

    if (!*renderer) {
        return TPXL_OUT_OF_MEMORY;
    }

    TpxlResult result = tpxl_init_kitty_context(&(*renderer)->kitty_context, context, frame, media_type);

    if (result != TPXL_OK) {
        free(*renderer);
        *renderer = NULL;
        return result;
    }

    return TPXL_OK;
}

TpxlResult tpxl_renderer_render(TpxlRenderer* renderer, TpxlImage* frame) {

    if (!renderer || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_render(&renderer->kitty_context, frame);
}

TpxlResult tpxl_renderer_upload(TpxlRenderer* renderer, TpxlImage* frame, uint32_t frame_id) {

    if (!renderer || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_transmit(&renderer->kitty_context, frame, frame_id);
}

TpxlResult tpxl_renderer_display(TpxlRenderer* renderer, uint32_t frame_id) {

    if (!renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_display(&renderer->kitty_context, frame_id);
}

void tpxl_destroy_renderer(TpxlRenderer* renderer) {

    if (!renderer) {
        return;
    }

    tpxl_destroy_kitty_context(&renderer->kitty_context);
    free(renderer);
}
