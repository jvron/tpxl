#ifndef TPXL_RENDERER_H
#define TPXL_RENDERER_H

#include "tpxl/type.h"
#include "tpxl/terminal.h"

typedef struct TpxlRendererImp TpxlRenderer;

typedef enum {
    TPXL_CURSOR_PRESERVE,
    TPXL_CURSOR_ADVANCE,
} TpxlCursorPolicy;

typedef enum {
    TPXL_BACKEND_AUTO = 0,
    TPXL_BACKEND_KITTY,
    TPXL_BACKEND_SIXEL,
    TPXL_BACKEND_COUNT,
} TpxlBackend;

typedef struct {
    TpxlTerminal* terminal;

    TpxlMediaType media_type;
    TpxlBackend backend;

    uint32_t render_width;
    uint32_t render_height;
    TpxlFormat format;
} TpxlRendererConfig;

TpxlResult tpxl_create_renderer(TpxlRenderer** renderer, TpxlRendererConfig* config);
TpxlResult tpxl_update_renderer_frame(TpxlRenderer* renderer, uint32_t render_width, uint32_t render_height, TpxlFormat format);
TpxlResult tpxl_update_renderer_media_policy(TpxlRenderer* renderer, TpxlMediaType media_type);

TpxlResult tpxl_renderer_direct_render(TpxlRenderer* renderer, TpxlImage* frame, uint32_t row, uint32_t column);
TpxlResult tpxl_renderer_upload(TpxlRenderer* renderer, TpxlImage* frame, uint32_t frame_id);
TpxlResult tpxl_renderer_display(TpxlRenderer* renderer, uint32_t frame_id, uint32_t row, uint32_t column);
void tpxl_renderer_delete_placement(TpxlRenderer* renderer, uint32_t frame_id);
void tpxl_renderer_delete_data(TpxlRenderer* renderer, uint32_t frame_id);
void tpxl_destroy_renderer(TpxlRenderer** renderer);

#endif
