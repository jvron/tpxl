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
    uint64_t duration;
    uint32_t frame_count;

    int video_stream_index;
    int video_stream_format;
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    AVPacket* av_packet;
    AVFrame* av_frame;
    struct SwsContext* sws_context;
    bool draining;

    TpxlAudio* audio;
};

struct TpxlVideoPlayerImp {
    TpxlRenderer* renderer;

    TpxlVideo* video;
    uint32_t frame_count;

    atomic_uint frame_id;
    bool playing;

    TpxlFrameIDQueue id_queue;
    TpxlFrameQueue frame_queue;

    atomic_bool shutdown;
    TpxlThreadStatus decode_status;
    TpxlThreadStatus upload1_status;
    TpxlThreadStatus upload2_status;

    pthread_t decode_thread;
    pthread_t upload_thread1;
    pthread_t upload_thread2;

    bool decode_thread_created;
    bool upload1_thread_created;
    bool upload2_thread_created;

    TpxlAudioPlayer* audio_player;
};

#endif
