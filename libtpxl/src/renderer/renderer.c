#include "kitty/kitty.h"
#include "tpxl/context.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

TpxlResult tpxl_render(TpxlContext* context, TpxlImage* image) {

    if (!context || !image) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_render(context, image);
}
