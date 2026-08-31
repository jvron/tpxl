#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "tpxl/animation.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

TpxlResult tpxl_init_animation_player(TpxlAnimationPlayer* player, TpxlAnimation* animation) {
    
    if (!player || !animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    player->animation = animation;
    player->current_frame = 0;
    player->elapsed = 0;

    return TPXL_OK;
}

bool tpxl_update_animation_player(TpxlAnimationPlayer* player, uint32_t delta) {

    bool changed = false;

    player->elapsed += delta;

    uint32_t delay = player->animation->delays[player->current_frame];

    while (player->elapsed >= delay) {
        
        changed = true;

        player->elapsed -= delay;

        player->current_frame++;

        if (player->current_frame >= player->animation->count) {
            player->current_frame = 0;
        }

        delay = player->animation->delays[player->current_frame];
    }
    return changed;
}

TpxlImage* tpxl_get_animation_frame(TpxlAnimationPlayer* player) {
    return &player->animation->frames[player->current_frame];
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

    *animation = (TpxlAnimation){0};
}

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
