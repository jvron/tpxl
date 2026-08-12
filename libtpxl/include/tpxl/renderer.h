#ifndef TPXL_RENDERER_H
#define TPXL_RENDERER_H

#include "tpxl/context.h"
#include "tpxl/type.h"

typedef struct TpxlRendererImp TpxlRenderer;

TpxlResult tpxl_create_renderer(TpxlRenderer** renderer, TpxlContext* context, TpxlImage* image, TpxlMediaType media_type);
TpxlResult tpxl_render(TpxlRenderer* renderer, TpxlImage* frame);
void tpxl_destroy_renderer(TpxlRenderer* renderer);

#endif
