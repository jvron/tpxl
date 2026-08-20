#ifndef TPXL_AUDIO_H
#define TPXL_AUDIO_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

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

    TpxlAudioFormat format;
    int64_t pts;
} TpxlAudioFrame;

TpxlResult tpxl_open_audio(const char* path, TpxlAudio** audio);
TpxlResult tpxl_decode_audio_frame(TpxlAudio* audio, TpxlAudioFrame* out_audio_frame);
void tpxl_free_audio_frame(TpxlAudioFrame* frame);
void tpxl_close_audio(TpxlAudio* audio);

TpxlResult tpxl_create_audio_player(TpxlAudioPlayer** player, TpxlAudio* audio);
TpxlResult tpxl_play_audio(TpxlAudioPlayer* player);
void tpxl_close_audio_player(TpxlAudioPlayer* player);

#endif
