#include <stdlib.h>
#include <stdbool.h>

#include "tpxl/image.h"
#include "tpxl/type.h"
#include "tpxl/video.h"

struct TpxlVideoPlayerImp {
    TpxlVideo* video;
    TpxlImage frame;

};

TpxlResult tpxl_create_video_player(TpxlVideo* video, TpxlVideoPlayer** player) {

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

    return TPXL_OK;
}

void tpxl_close_video_player(TpxlVideoPlayer* player) {

    if (!player) {
        return;
    }

    player->video = NULL;

    tpxl_free_frame(&player->frame);
    free(player);
}
