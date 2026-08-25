#ifndef TPXL_AUDIO_INTERNAL_H
#define TPXL_AUDIO_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/rational.h>
#include <libswresample/swresample.h>
#include <miniaudio/miniaudio.h>

#include "tpxl/audio.h"
#include "queue/queue.h"

#include "thread.h"
#include "video_internal.h"

struct TpxlAudioImp {
    uint64_t duration;

    int audio_stream_index;
    int sample_rate;
    AVRational time_base;
    AVChannelLayout channel_layout;
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    AVPacket* av_packet;
    AVFrame* av_frame;

    // Output format
    int output_sample_rate;
    AVChannelLayout output_channel_layout;
    enum AVSampleFormat output_format;
    SwrContext* swr_context;

    bool draining;
    bool drain_sent;
};

struct TpxlAudioPlayerImp {
    TpxlAudio* audio;

    ma_device device;
    atomic_bool playing;

    TpxlAudioFrame current_frame;
    size_t current_frame_position;

    atomic_uint_fast64_t frames_played;

    TpxlAudioFrameQueue frame_queue;

    atomic_bool shutdown;
    pthread_t decode_thread;
    TpxlThreadStatus decode_status;
};

TpxlResult tpxl_init_video_audio(TpxlAudio** audio, AVFormatContext* format_context);
TpxlResult tpxl_decode_audio_packet(TpxlAudio* audio, AVPacket* packet, TpxlAudioFrame* out_audio_frame);
TpxlResult tpxl_init_video_audio_player(TpxlVideoPlayer* video_player, TpxlAudio* audio);

#endif
