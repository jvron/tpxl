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

TpxlResult tpxl_video_open(const char* path, TpxlVideo** video);
TpxlResult tpxl_video_set_output_size(TpxlVideo* video, uint32_t width, uint32_t height);
TpxlResult tpxl_video_get_source_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height);
TpxlResult tpxl_video_get_output_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height);
TpxlFormat tpxl_video_get_format(TpxlVideo* video);
TpxlResult tpxl_video_get_attached_thumbnail(TpxlVideo* video, TpxlImage* image);
TpxlResult tpxl_video_view_attached_thumbnail(TpxlVideo* video, const TpxlImage** image);
uint32_t tpxl_video_get_frame_count(TpxlVideo* video);
double tpxl_video_get_frame_rate(TpxlVideo* video);
double tpxl_video_get_duration(TpxlVideo* video);
TpxlResult tpxl_video_print_info(const TpxlVideo* video);
void tpxl_video_frame_free(TpxlVideoFrame* video_frame);
void tpxl_video_close(TpxlVideo** video);

TpxlResult tpxl_video_player_create(TpxlVideoPlayer** player, TpxlRenderer* renderer, TpxlVideo* video);
TpxlResult tpxl_video_player_update(TpxlVideoPlayer* player);
TpxlResult tpxl_video_player_start(TpxlVideoPlayer* player, uint32_t row, uint32_t column);
TpxlResult tpxl_video_play(TpxlVideoPlayer* player);
TpxlResult tpxl_video_pause(TpxlVideoPlayer* player);
TpxlResult tpxl_video_mute(TpxlVideoPlayer* player);
TpxlResult tpxl_video_unmute(TpxlVideoPlayer* player);
bool tpxl_video_player_active(TpxlVideoPlayer* player);
bool tpxl_video_player_playing(TpxlVideoPlayer* player);
bool tpxl_video_player_muted(TpxlVideoPlayer* player);
double tpxl_video_player_get_time(TpxlVideoPlayer* player);
uint32_t tpxl_video_player_get_frames_played(TpxlVideoPlayer* player);
void tpxl_video_player_destroy(TpxlVideoPlayer** player);

#endif
