#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "tpxl/animation.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

TpxlResult tpxl_print_animation_info(TpxlAnimation* animation) {

    if (!animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    printf("Width: %d\n", animation->width);
    printf("Height: %d\n", animation->height);
    printf("Format: %s\n", tpxl_format_to_string(animation->format));
    printf("Frame count: %lu\n", animation->count);

    return TPXL_OK;
}

TpxlResult tpxl_init_animator(TpxlAnimator* animator, TpxlAnimation* animation) {
    
    if (!animator || !animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    animator->animation = animation;
    animator->current_frame = 0;
    animator->elapsed = 0;

    return TPXL_OK;
}

bool tpxl_update_animator(TpxlAnimator* animator, uint32_t delta) {

    bool changed = false;

    animator->elapsed += delta;

    uint32_t delay = animator->animation->delays[animator->current_frame];

    while (animator->elapsed >= delay) {
        
        changed = true;

        animator->elapsed -= delay;

        animator->current_frame++;

        if (animator->current_frame >= animator->animation->count) {
            animator->current_frame = 0;
        }

        delay = animator->animation->delays[animator->current_frame];
    }
    return changed;
}

TpxlImage* tpxl_get_animation_frame(TpxlAnimator* animator) {
    return &animator->animation->frames[animator->current_frame];
}

void tpxl_free_animation(TpxlAnimation* animation) {

    if (!animation) {
        return;
    }

    for (size_t i = 0; i < animation->count; i++) {
        tpxl_free_frame(&animation->frames[i]);
    }

    free(animation->frames);
    free(animation->delays);

    animation->count = 0;
    animation->frames = NULL;
    animation->delays = NULL;
}
