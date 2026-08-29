#ifndef TPXL_VIDEO_INTERNAL_H
#define TPXL_VIDEO_INTERNAL_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/rational.h>

#include "tpxl/renderer.h"
#include "tpxl/type.h"
#include "tpxl/audio.h"
#include "tpxl/video.h"
#include "queue/queue.h"

#include "thread.h"

struct TpxlVideoImp {
    uint32_t width;
    uint32_t height;
    uint32_t output_width;
    uint32_t output_height;
    TpxlFormat format;
    uint32_t frame_count;

    int video_stream_index;
    int video_stream_format;

    AVRational time_base;

    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    struct SwsContext* sws_context;
    AVFrame* av_frame;

    bool drain_sent;

    TpxlAudio* audio;
};

struct TpxlVideoPlayerImp {
    TpxlRenderer* renderer;

    TpxlVideo* video;
    uint32_t frame_count;

    atomic_uint frame_id;
    atomic_bool playing;

    TpxlVideoFrame current_frame;
    bool has_current_frame;

    uint32_t previous_frame_id;
    bool has_previous_frame;

    TpxlPacketQueue video_packet_queue;
    TpxlPacketQueue audio_packet_queue;

    TpxlVideoFrameQueue upload_queue;
    TpxlVideoFrameQueue display_queue;

    atomic_bool shutdown;
    _Atomic TpxlThreadStatus demux_status;
    TpxlThreadStatus decode_status;
    TpxlThreadStatus upload_status;
    TpxlThreadStatus play_status;

    pthread_t demux_thread;
    pthread_t decode_thread;
    pthread_t upload_thread;
    pthread_t play_thread;

    bool demux_thread_created;
    bool decode_thread_created;
    bool upload_thread_created;
    bool play_thread_created;

    TpxlAudioPlayer* audio_player;
    atomic_bool frame_ready;
};

TpxlResult tpxl_decode_video_packet(TpxlVideo* video, AVPacket* packet, TpxlVideoFrame* out_frame);

#endif
