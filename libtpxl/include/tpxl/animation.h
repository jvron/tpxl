#ifndef TPXL_ANIMATION_H
#define TPXL_ANIMATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "tpxl/type.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    
    TpxlImage* frames;
    uint32_t* delays;
    size_t count;
} TpxlAnimation;

typedef struct {
    TpxlAnimation* animation;
    size_t current_frame;
    uint32_t elapsed; 
} TpxlAnimationPlayer;

TpxlResult tpxl_load_gif(const char* path, TpxlAnimation* animation);

TpxlResult tpxl_init_animation_player(TpxlAnimationPlayer* player, TpxlAnimation* animation);
bool tpxl_update_animation_player(TpxlAnimationPlayer* player, uint32_t delta);
TpxlImage* tpxl_get_animation_frame(TpxlAnimationPlayer* player);
void tpxl_free_animation(TpxlAnimation* animation);
TpxlResult tpxl_print_animation_info(TpxlAnimation* animation);

#endif
