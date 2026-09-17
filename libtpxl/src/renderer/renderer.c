#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "tpxl/context.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

#include "kitty/kitty_backend.h"
#include "sixel/sixel_backend.h"

struct TpxlRendererImp {
    TpxlMediaType media_type;
    TpxlBackend backend;

    union {
        TpxlKittyContext kitty_context;
        TpxlSixelContext sixel_context;
    };
};

TpxlResult tpxl_create_renderer(TpxlRenderer** renderer, TpxlContext* context, uint32_t width, uint32_t height, TpxlFormat format, TpxlMediaType media_type) {

    if (!renderer || !context) {
        return TPXL_INVALID_ARGUMENT;
    }

    *renderer = malloc(sizeof(TpxlRenderer));

    if (!*renderer) {
        return TPXL_OUT_OF_MEMORY;
    }

    TpxlResult result = TPXL_OK;

    switch (context->backend) {

        case TPXL_BACKEND_KITTY:
            (*renderer)->backend = TPXL_BACKEND_KITTY;
            (*renderer)->kitty_context = (TpxlKittyContext){0};

            result = tpxl_set_kitty_context(&(*renderer)->kitty_context, context);

            if (result != TPXL_OK) {
                free(*renderer);
                *renderer = NULL;
                return result;
            }

            result = tpxl_set_kitty_media_policy(&(*renderer)->kitty_context, media_type);

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

            break;

        case TPXL_BACKEND_SIXEL:
            (*renderer)->backend = TPXL_BACKEND_SIXEL;
            (*renderer)->sixel_context = (TpxlSixelContext){0};

            result = tpxl_set_sixel_context(&(*renderer)->sixel_context, context, true);

            if (result != TPXL_OK) {
                free(*renderer);
                *renderer = NULL;
                return result;
            }

            result = tpxl_set_sixel_media_policy(&(*renderer)->sixel_context, media_type);

            if (result != TPXL_OK) {
                free(*renderer);
                *renderer = NULL;
                return result;
            }

            result = tpxl_set_sixel_frame(&(*renderer)->sixel_context, width, height, format);

            if (result != TPXL_OK) {
                free(*renderer);
                *renderer = NULL;
                return result;
            }

            break;

        default:
            free(*renderer);
            return TPXL_INVALID_BACKEND;
    }

    return TPXL_OK;
}

TpxlResult tpxl_update_renderer_context(TpxlRenderer* renderer, TpxlContext* context) {

    if (!renderer || !context) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (renderer->backend) {
        case TPXL_BACKEND_KITTY:
            return tpxl_set_kitty_context(&renderer->kitty_context, context);
        case TPXL_BACKEND_SIXEL:
            return tpxl_set_sixel_context(&renderer->sixel_context, context, false);

        default:
            return TPXL_INVALID_BACKEND;
    }   
}

TpxlResult tpxl_update_renderer_frame(TpxlRenderer* renderer, uint32_t width, uint32_t height, TpxlFormat format) {

    if (!renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (renderer->backend) {
        case TPXL_BACKEND_KITTY:
            return tpxl_set_kitty_frame(&renderer->kitty_context, width, height, format);
        case TPXL_BACKEND_SIXEL:
            return tpxl_set_sixel_frame(&renderer->sixel_context, width, height, format);

        default:
            return TPXL_INVALID_BACKEND;
    } 
}

TpxlResult tpxl_update_renderer_media_policy(TpxlRenderer* renderer, TpxlMediaType media_type) {

    if (!renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (renderer->backend) {
        case TPXL_BACKEND_KITTY:
            return tpxl_set_kitty_media_policy(&renderer->kitty_context, media_type);
        case TPXL_BACKEND_SIXEL:
            return tpxl_set_sixel_media_policy(&renderer->sixel_context, media_type);

        default:
            return TPXL_INVALID_BACKEND;
    } 
}

TpxlResult tpxl_renderer_render(TpxlRenderer* renderer, TpxlImage* frame) {

    if (!renderer || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (renderer->backend) {
        case TPXL_BACKEND_KITTY:
            return tpxl_kitty_render(&renderer->kitty_context, frame);
        case TPXL_BACKEND_SIXEL:
            return tpxl_sixel_render(&renderer->sixel_context, frame);

        default:
            return TPXL_INVALID_BACKEND;
    } 
}

TpxlResult tpxl_renderer_upload(TpxlRenderer* renderer, TpxlImage* frame, uint32_t frame_id) {

    if (!renderer || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (renderer->backend) {
        case TPXL_BACKEND_KITTY:
            return tpxl_kitty_transmit(&renderer->kitty_context, frame, frame_id);
        case TPXL_BACKEND_SIXEL:
            return tpxl_sixel_upload(&renderer->sixel_context, frame, frame_id);

        default:
            return TPXL_INVALID_BACKEND;
    } 
}

TpxlResult tpxl_renderer_display(TpxlRenderer* renderer, uint32_t frame_id) {

    if (!renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    switch (renderer->backend) {
        case TPXL_BACKEND_KITTY:
            return tpxl_kitty_display(&renderer->kitty_context, frame_id);
        case TPXL_BACKEND_SIXEL:
            return tpxl_sixel_display(&renderer->sixel_context, frame_id);

        default:
            return TPXL_INVALID_BACKEND;
    } 
}

void tpxl_renderer_delete_placement(TpxlRenderer* renderer, uint32_t frame_id) {

    if (!renderer) {
        return;
    }

    if (renderer->backend == TPXL_BACKEND_KITTY) {
        tpxl_kitty_delete_placement(frame_id);
    } else if (renderer->backend == TPXL_BACKEND_SIXEL) {
        tpxl_sixel_delete_placement(&renderer->sixel_context, frame_id);
    }
}

void tpxl_renderer_delete_data(TpxlRenderer* renderer, uint32_t frame_id) {

    if (!renderer) {
        return;
    }

    if (renderer->backend == TPXL_BACKEND_KITTY) {
        tpxl_kitty_delete_data(frame_id);
    } else if (renderer->backend == TPXL_BACKEND_SIXEL) {
        tpxl_sixel_delete_data(&renderer->sixel_context, frame_id);
    }
}

void tpxl_destroy_renderer(TpxlRenderer** renderer) {

    if (!*renderer) {
        return;
    }

    if ((*renderer)->backend == TPXL_BACKEND_KITTY) {
        tpxl_destroy_kitty_context(&(*renderer)->kitty_context);
    } else if ((*renderer)->backend == TPXL_BACKEND_SIXEL) {
        tpxl_destroy_sixel_context(&(*renderer)->sixel_context);
    }
    
    free(*renderer);
    *renderer = NULL;
}
