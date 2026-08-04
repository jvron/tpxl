#include "kitty/kitty.h"
#include "tpxl/context.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

TpxlResult tpxl_render(TpxlContext* context, TpxlImage* image, TpxlMediaType media_type) {

    if (!context || !image) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_render(context, image, media_type);
}
