#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <stdint.h>
#include <stdio.h>

#include "tpxl/video.h"
#include "tpxl/type.h"

struct TpxlVideoImp {
    uint32_t width;
    uint32_t height;
    uint64_t duration;

    uint32_t video_stream_index;
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
};

TpxlResult tpxl_open_video(const char* path, TpxlVideo** video) {

    if (!path || !video) {
        return TPXL_INVALID_ARGUMENT;
    }

    AVFormatContext* format_context = NULL;

    if (avformat_open_input(&format_context, path, NULL, NULL) < 0) {
        return TPXL_VIDEO_LOAD_FAILED;
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        avformat_close_input(&format_context);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    AVStream* video_stream = NULL;
    int video_stream_index;

    for (size_t i = 0; i < format_context->nb_streams; i++) {

        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            video_stream = format_context->streams[i];
            break;
        }
    }

    if (!video_stream) {
        avformat_close_input(&format_context);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);

    if (!codec) {
        avformat_close_input(&format_context);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);

    if (!codec_context) {
        avformat_close_input(&format_context);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    if (avcodec_parameters_to_context(codec_context, video_stream->codecpar) < 0) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    *video = malloc(sizeof(TpxlVideo));
    
    if (!*video) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_OUT_OF_MEMORY;
    }

    (*video)->width = video_stream->codecpar->width;
    (*video)->height = video_stream->codecpar->height;
    (*video)->duration = video_stream->duration;
    (*video)->video_stream_index = video_stream_index;
    (*video)->format_context = format_context;
    (*video)->codec_context = codec_context;

    return TPXL_OK;
}

void tpxl_close_video(TpxlVideo* video) {

    if (!video) {
        return;
    }

    avcodec_free_context(&video->codec_context);
    avformat_close_input(&video->format_context);

    free(video);
}
