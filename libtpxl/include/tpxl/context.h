#ifndef TPXL_CONTEXT_H
#define TPXL_CONTEXT_H

#include <stdint.h>

#include "type.h"
#include "terminal.h"
#include "viewport.h"

typedef enum {
    TPXL_BACKEND_AUTO = 0,
    TPXL_BACKEND_KITTY,
    TPXL_BACKEND_SIXEL,

} TpxlBackend;

typedef enum {
    TPXL_SCALE_NONE = 0,
    TPXL_SCALE_FIT,
    TPXL_SCALE_FILL,

} TpxlScaleMode;

typedef struct {
    TpxlTerminal terminal;
    TpxlViewport viewport;

    TpxlBackend backend;
    TpxlScaleMode scale_mode;

} TpxlContext;

TpxlResult tpxl_init_context(TpxlContext* context);

TpxlResult tpxl_set_viewport(TpxlContext* context, const TpxlViewport* viewport);
TpxlResult tpxl_update_context(TpxlContext* context);
TpxlResult tpxl_update_viewport(TpxlContext* context, uint32_t content_width, uint32_t content_height);

#endif
