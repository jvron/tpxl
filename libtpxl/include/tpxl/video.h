#ifndef TPXL_VIDEO_H
#define TPXL_VIDEO_H

#include <stdint.h>
#include <stdbool.h>

#include "tpxl/renderer.h"
#include "tpxl/type.h"

typedef struct TpxlVideoImp TpxlVideo;
typedef struct TpxlVideoPlayerImp TpxlVideoPlayer;

typedef struct {
    TpxlImage frame;
    uint32_t id;
    int64_t pts;

} TpxlVideoFrame;

TpxlResult tpxl_open_video(const char* path, TpxlVideo** video);
TpxlResult tpxl_video_set_output_size(TpxlVideo* video, uint32_t width, uint32_t height);
TpxlResult tpxl_get_video_source_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height);
TpxlResult tpxl_get_video_output_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height);
TpxlResult tpxl_get_video_format(TpxlVideo* video, TpxlFormat* format);
uint32_t tpxl_get_video_frame_count(TpxlVideo* video);
void tpxl_free_video_frame(TpxlVideoFrame* video_frame);
void tpxl_close_video(TpxlVideo* video);

TpxlResult tpxl_create_video_player(TpxlVideoPlayer** player, TpxlRenderer* renderer, TpxlVideo* video);
TpxlResult tpxl_update_video_player(TpxlVideoPlayer* player, TpxlRenderer* renderer);
TpxlResult tpxl_play_video(TpxlVideoPlayer* player, TpxlRenderer* renderer);
bool tpxl_video_playing(TpxlVideoPlayer* player);
void tpxl_close_video_player(TpxlVideoPlayer* player);

#endif
