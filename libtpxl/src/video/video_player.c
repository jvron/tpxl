#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tpxl/type.h"
#include "tpxl/video.h"
#include "frame_queue.h"

struct TpxlVideoPlayerImp {
    TpxlVideo* video;
    TpxlFrameQueue queue;
    uint32_t frame_id;
    bool playing;
};

TpxlResult tpxl_create_video_player(TpxlVideoPlayer** player, TpxlVideo* video) {

    if (!video || !player) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = malloc(sizeof(TpxlVideoPlayer));

    if (!*player) {
        return TPXL_OUT_OF_MEMORY;
    }

    (*player)->video = video;
    (*player)->queue = (TpxlFrameQueue){0};
    (*player)->frame_id = 1;
    (*player)->playing = true;

    return TPXL_OK;
}

TpxlResult tpxl_update_video_player(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlImage frame = {0};
    TpxlResult result = tpxl_decode_video_frame(player->video, &frame);

    if (result == TPXL_EOF) {
        player->playing = false;
        return TPXL_OK;
    }
    if (result != TPXL_OK) {
        return result;
    }

    result = tpxl_frame_queue_push(&player->queue, &frame);

    if (result != TPXL_OK) {
        return result;
    }
    
    player->frame_id++;

    return TPXL_OK;
}

TpxlResult tpxl_video_player_pop_frame(TpxlVideoPlayer* player, TpxlImage* out_frame) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_frame_queue_pop(&player->queue, out_frame);
}

bool tpxl_video_playing(TpxlVideoPlayer* player) {

    if (!player) {
        return false;
    }

    return player->playing;
}

void tpxl_close_video_player(TpxlVideoPlayer* player) {

    if (!player) {
        return;
    }

    player->video = NULL;
    free(player);
    player = NULL;
}
