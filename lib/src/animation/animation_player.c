#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "tpxl/animation.h"
#include "tpxl/renderer.h"
#include "tpxl/type.h"
#include "tpxl/util.h"

#include "internal/thread.h"

struct TpxlAnimationPlayerImp {
    TpxlAnimation* animation;

    TpxlRenderer* renderer;
    TpxlBackend backend;

    uint32_t row;
    uint32_t column;

    uint32_t previous_frame_id;
    bool has_previous_frame;

    atomic_bool active;
    atomic_bool playing;

    pthread_t play_thread;
    bool play_thread_created;
    int play_status;
};

static void* tpxl_animation_play_worker(void* arg) {

    TpxlAnimationPlayer* player = arg;
    
    player->play_status = TPXL_THREAD_RUNNING;

    TpxlAnimation* animation = player->animation;

    while (atomic_load(&player->active)) {

        for (size_t i = 0; i < animation->frame_count; i++) {

            while (!atomic_load(&player->playing) && atomic_load(&player->active)) {
                tpxl_sleep_ms(100);
            }

            if (!atomic_load(&player->active)) {
                break;
            }
            
            if (player->has_previous_frame && player->backend == TPXL_BACKEND_KITTY) {
                tpxl_renderer_delete_placement(player->renderer, player->previous_frame_id);
            }

            TpxlAnimationFrame* animation_frame = &animation->frames[i];

            TpxlResult result = tpxl_renderer_display(
                player->renderer, 
                animation_frame->frame_id, 
                player->row, 
                player->column
            );

            player->previous_frame_id = animation_frame->frame_id;
            player->has_previous_frame = true;

            if (result != TPXL_OK) {
                player->play_status = TPXL_THREAD_ERROR;
                break;
            }

            tpxl_sleep_ms(animation_frame->delay);
        } 
    }

    if (player->play_status != TPXL_THREAD_ERROR) {
        player->play_status = TPXL_THREAD_FINISHED;
    }

    return NULL;
}

TpxlResult tpxl_create_animation_player(TpxlAnimationPlayer** player, TpxlRenderer* renderer, TpxlAnimation* animation) {
    
    if (!player || !renderer || !animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = NULL;

    TpxlAnimationPlayer* new_player = calloc(1,  sizeof(TpxlAnimationPlayer));

    if (!new_player) {
        return TPXL_OUT_OF_MEMORY;
    }

    new_player->renderer = renderer;
    new_player->backend = tpxl_get_renderer_backend(renderer);

    new_player->animation = animation;

    new_player->row = 0;
    new_player->column = 0;

    new_player->previous_frame_id = 0;
    new_player->has_previous_frame = false;

    new_player->play_thread_created = false;

    atomic_init(&new_player->playing, false);
    atomic_init(&new_player->active, false);
   
    for (size_t i = 0; i < animation->frame_count; i++) {
        
        uint32_t id = i + 1;

        TpxlResult result = tpxl_renderer_upload(renderer, &animation->frames[i].frame, id);

        if (result != TPXL_OK) {
            free(new_player);
            return TPXL_ANIMATION_PLAYER_CREATION_FAILED;
        }

        animation->frames[i].frame_id = id;
    }

    atomic_store(&new_player->active, true);

    *player = new_player;

    return TPXL_OK;
}

TpxlResult tpxl_play_animation(TpxlAnimationPlayer* player, uint32_t row, uint32_t column) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (player->play_thread_created) {
        return TPXL_OK;
    }

    player->row = row;
    player->column = column;

    atomic_store(&player->playing, true);

    if (pthread_create(&player->play_thread, NULL, tpxl_animation_play_worker, player) != 0) {
        atomic_store(&player->playing, false);
        return TPXL_THREAD_CREATION_ERROR;
    }

    player->play_thread_created = true;

    return TPXL_OK;
}

void tpxl_close_animation_player(TpxlAnimationPlayer** player) {

    if (!*player) {
        return;
    }

    atomic_store(&(*player)->active, false);

    if ((*player)->play_thread_created) {
        pthread_join((*player)->play_thread, NULL);
    }

    free(*player);
    *player = NULL;
}


