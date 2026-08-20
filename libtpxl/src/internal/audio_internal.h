#ifndef TPXL_AUDIO_INTERNAL_H
#define TPXL_AUDIO_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/rational.h>
#include <libswresample/swresample.h>

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
};

#endif
