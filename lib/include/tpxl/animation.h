#ifndef TPXL_ANIMATION_H
#define TPXL_ANIMATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/renderer.h"

typedef struct TpxlAnimationPlayerImp TpxlAnimationPlayer;

typedef struct {
    TpxlImage frame;
    uint32_t delay;
    uint32_t frame_id;
} TpxlAnimationFrame;

typedef struct {
    uint32_t width;
    uint32_t height;

    uint32_t output_width;
    uint32_t output_height;
    TpxlFormat format;

    TpxlAnimationFrame* frames;
    size_t frame_count;
    size_t frame_capacity;
}  TpxlAnimation;

TpxlResult tpxl_animation_load(const char* path, TpxlAnimation* animation);
TpxlResult tpxl_animation_resize(TpxlAnimation* animation, uint32_t output_width, uint32_t output_height);
void tpxl_animation_free(TpxlAnimation* animation);
TpxlResult tpxl_animation_print_info(const TpxlAnimation* animation);

TpxlResult tpxl_animation_player_create(TpxlAnimationPlayer** player, TpxlRenderer* renderer, TpxlAnimation* animation);
TpxlResult tpxl_animation_play(TpxlAnimationPlayer* player, uint32_t row, uint32_t column);
void tpxl_animation_player_destroy(TpxlAnimationPlayer** player);

#endif