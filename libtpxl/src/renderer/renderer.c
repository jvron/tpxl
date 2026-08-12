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

TpxlResult tpxl_render(TpxlRenderer* renderer, TpxlImage* frame) {

    if (!renderer || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_render(&renderer->kitty_context, frame);
}

void tpxl_destroy_renderer(TpxlRenderer* renderer) {

    if (!renderer) {
        return;
    }

    tpxl_destroy_kitty_context(&renderer->kitty_context);
    free(renderer);
}
