#include <stdint.h>
#include <stdlib.h>

#include "kitty/kitty.h"
#include "tpxl/context.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

struct TpxlRendererImp {
    TpxlKittyContext kitty_context;
};

TpxlResult tpxl_create_renderer(TpxlRenderer** renderer, TpxlContext* context, uint32_t width, uint32_t height, TpxlFormat format, TpxlMediaType media_type) {

    if (!renderer || !context) {
        return TPXL_INVALID_ARGUMENT;
    }

    *renderer = malloc(sizeof(TpxlRenderer));

    if (!*renderer) {
        return TPXL_OUT_OF_MEMORY;
    }

    (*renderer)->kitty_context = (TpxlKittyContext){0};

    TpxlResult result = tpxl_set_kitty_context(&(*renderer)->kitty_context, context);

    if (result != TPXL_OK) {
        free(*renderer);
        *renderer = NULL;
        return result;
    }

    result = tpxl_set_kitty_cursor_policy(&(*renderer)->kitty_context, media_type);

    if (result != TPXL_OK) {
        free(*renderer);
        *renderer = NULL;
        return result;
    }

    result = tpxl_set_kitty_frame(&(*renderer)->kitty_context, width, height, format);

    if (result != TPXL_OK) {
        free(*renderer);
        *renderer = NULL;
        return result;
    }

    return TPXL_OK;
}

TpxlResult tpxl_update_renderer_context(TpxlRenderer* renderer, TpxlContext* context) {

    if (!renderer || !context) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_set_kitty_context(&renderer->kitty_context, context);
}

TpxlResult tpxl_update_renderer_frame(TpxlRenderer* renderer, uint32_t width, uint32_t height, TpxlFormat format) {

    if (!renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_set_kitty_frame(&renderer->kitty_context, width, height, format);
}

TpxlResult tpxl_update_renderer_media_policy(TpxlRenderer* renderer, TpxlMediaType media_type) {

    if (!renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_set_kitty_cursor_policy(&renderer->kitty_context, media_type);
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
    renderer = NULL;
}
