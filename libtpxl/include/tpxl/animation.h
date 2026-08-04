#ifndef TPXL_ANIMATION_H
#define TPXL_ANIMATION_H

#include "tpxl/type.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    TpxlAnimation* animation;
    size_t current_frame;
    uint32_t elapsed; 

} TpxlAnimator;

TpxlResult tpxl_load_gif(const char* path, TpxlAnimation* animation);

bool tpxl_update_animation(TpxlAnimator* animator, uint32_t delta);
TpxlImage* tpxl_get_animation_frame(TpxlAnimator* animator);

void tpxl_free_animation(TpxlAnimation* animation);

#endif
