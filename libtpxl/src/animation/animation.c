#include <stdlib.h>

#include "tpxl/animation.h"
#include "tpxl/image.h"

void tpxl_free_animation(TpxlAnimation* animation) {

    if (!animation) {
        return;
    }

    for (size_t i = 0; i < animation->count; i++) {
        tpxl_free_image(&animation->frames[i]);
    }

    free(animation->frames);
    free(animation->delays);

    animation->frames = NULL;
    animation->delays = NULL;
    animation->count = 0;
}
