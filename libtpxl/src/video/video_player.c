#include <stdlib.h>
#include <stdbool.h>

#include "tpxl/image.h"
#include "tpxl/type.h"
#include "tpxl/video.h"

struct TpxlVideoPlayerImp {
    TpxlVideo* video;
    TpxlImage frame;
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
    (*player)->frame = (TpxlImage){
        .width = 0,
        .height = 0,
        .format = TPXL_FORMAT_UNKNOWN,
        .pixels = NULL
    };

    (*player)->playing = true;

    return TPXL_OK;
}

TpxlResult tpxl_update_video_player(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = tpxl_decode_video_frame(player->video, &player->frame);

    if (result == TPXL_EOF) {
        player->playing = false;
        return TPXL_OK;
    }

    return result;
}

TpxlImage* tpxl_video_player_get_frame(TpxlVideoPlayer* player) {

    if (!player) {
        return NULL;
    }

    return &player->frame;
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

    tpxl_free_frame(&player->frame);
    free(player);
}
