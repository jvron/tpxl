#include <stdint.h>
#include <stdlib.h>

#include "tpxl/animation.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

void tpxl_update_animation(TpxlAnimator* animator, uint32_t delta) {

    animator->elapsed += delta;

    uint32_t delay = animator->animation->delays[animator->current_frame];

    if (animator->elapsed >= delay) {

        animator->elapsed = 0;

        animator->current_frame++;

        if (animator->current_frame >= animator->animation->count) {
            animator->current_frame = 0;
        }
    }
}

TpxlImage* tpxl_get_animation_frame(TpxlAnimator* animator) {
    return &animator->animation->frames[animator->current_frame];
}

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
