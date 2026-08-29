#ifndef TPXL_RENDERER_H
#define TPXL_RENDERER_H

#include "tpxl/context.h"
#include "tpxl/type.h"
#include <stdint.h>

typedef struct TpxlRendererImp TpxlRenderer;

TpxlResult tpxl_create_renderer(TpxlRenderer** renderer, TpxlContext* context, uint32_t width, uint32_t height, TpxlFormat format, TpxlMediaType media_type);
TpxlResult tpxl_update_renderer_context(TpxlRenderer* renderer, TpxlContext* context);
TpxlResult tpxl_update_renderer_frame(TpxlRenderer* renderer, uint32_t width, uint32_t height, TpxlFormat format);
TpxlResult tpxl_update_renderer_media_policy(TpxlRenderer* renderer, TpxlMediaType media_type);

TpxlResult tpxl_renderer_render(TpxlRenderer* renderer, TpxlImage* frame);
TpxlResult tpxl_renderer_upload(TpxlRenderer* renderer, TpxlImage* frame, uint32_t frame_id);
TpxlResult tpxl_renderer_display(TpxlRenderer* renderer, uint32_t frame_id);
void tpxl_renderer_delete(uint32_t frame_id);
void tpxl_destroy_renderer(TpxlRenderer* renderer);

#endif
