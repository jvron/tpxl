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

TpxlResult tpxl_load_animation(const char* path, TpxlAnimation* animation);
TpxlResult tpxl_resize_animation(TpxlAnimation* animation, uint32_t output_width, uint32_t output_height);
void tpxl_free_animation(TpxlAnimation* animation);
TpxlResult tpxl_print_animation_info(TpxlAnimation* animation);

TpxlResult tpxl_create_animation_player(TpxlAnimationPlayer** player, TpxlRenderer* renderer, TpxlAnimation* animation);
TpxlResult tpxl_play_animation(TpxlAnimationPlayer* player, uint32_t row, uint32_t column);
void tpxl_close_animation_player(TpxlAnimationPlayer** player);

#endif