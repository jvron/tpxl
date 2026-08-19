#ifndef TPXL_VIDEO_INTERNAL_H
#define TPXL_VIDEO_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/rational.h>

#include "tpxl/type.h"
#include "tpxl/audio.h"

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

#endif
