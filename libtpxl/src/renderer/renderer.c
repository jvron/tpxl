#include "kitty/kitty.h"
#include "tpxl/type.h"
#include "tpxl/renderer.h"

TpxlResult tpxl_render(TpxlImage* image) {

    if (!image) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_kitty_render(image);
}
