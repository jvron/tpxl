#ifndef TPXL_VIDEO_H
#define TPXL_VIDEO_H

#include <stdint.h>

#include "tpxl/type.h"

typedef struct TpxlVideoImp TpxlVideo;
typedef struct TpxlVideoPlayerImp TpxlVideoPlayer;

TpxlResult tpxl_open_video(const char* path, TpxlVideo** video);
TpxlResult tpxl_get_video_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height);
TpxlResult tpxl_decode_video_frame(TpxlVideo* video, TpxlImage* frame);
void tpxl_close_video(TpxlVideo* video);

TpxlResult tpxl_create_video_player(TpxlVideo* video, TpxlVideoPlayer** player);
void tpxl_close_video_player(TpxlVideoPlayer* player);

#endif
