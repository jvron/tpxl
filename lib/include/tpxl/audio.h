#ifndef TPXL_AUDIO_H
#define TPXL_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "tpxl/type.h"

typedef struct TpxlAudioImp TpxlAudio;
typedef struct TpxlAudioPlayerImp TpxlAudioPlayer;

typedef enum {
    TPXL_AUDIO_FORMAT_UNKNOWN = 0,

    TPXL_AUDIO_FORMAT_U8,
    TPXL_AUDIO_FORMAT_S16,
    TPXL_AUDIO_FORMAT_S32,
    TPXL_AUDIO_FORMAT_S64,

    TPXL_AUDIO_FORMAT_FLT,
    TPXL_AUDIO_FORMAT_DBL,

    TPXL_AUDIO_FORMAT_U8P,
    TPXL_AUDIO_FORMAT_S16P,
    TPXL_AUDIO_FORMAT_S32P,
    TPXL_AUDIO_FORMAT_S64P,

    TPXL_AUDIO_FORMAT_FLTP,
    TPXL_AUDIO_FORMAT_DBLP,
} TpxlAudioFormat;

typedef struct {
    uint8_t* samples;
    size_t sample_count;
    int64_t pts;
    TpxlAudioFormat format;
} TpxlAudioFrame;

TpxlResult tpxl_audio_open(const char* path, TpxlAudio** audio);
double tpxl_audio_get_duration(TpxlAudio* audio);
TpxlResult tpxl_audio_get_attached_thumbnail(TpxlAudio* audio, TpxlImage* image);
TpxlResult tpxl_audio_view_attached_thumbnail(TpxlAudio* audio, const TpxlImage** image);
TpxlResult tpxl_audio_decode_frame(TpxlAudio* audio, TpxlAudioFrame* out_audio_frame);
void tpxl_audio_frame_free(TpxlAudioFrame* frame);
void tpxl_audio_close(TpxlAudio** audio);
void tpxl_video_audio_close(TpxlAudio** audio);

TpxlResult tpxl_audio_player_create(TpxlAudioPlayer** player, TpxlAudio* audio);
TpxlResult tpxl_audio_play(TpxlAudioPlayer* player);
TpxlResult tpxl_audio_pause(TpxlAudioPlayer* player);
TpxlResult tpxl_audio_mute(TpxlAudioPlayer* player);
TpxlResult tpxl_audio_unmute(TpxlAudioPlayer* player);
double tpxl_audio_player_get_time(TpxlAudioPlayer* player);
bool tpxl_audio_player_active(TpxlAudioPlayer* player);
bool tpxl_audio_player_playing(TpxlAudioPlayer* player);
bool tpxl_audio_player_muted(TpxlAudioPlayer* player);
void tpxl_audio_player_destroy(TpxlAudioPlayer** player);

#endif
