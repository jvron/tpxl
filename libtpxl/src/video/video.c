#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>

#include "tpxl/video.h"
#include "tpxl/type.h"

struct TpxlVideoImp {
    uint32_t width;
    uint32_t height;
    uint64_t duration;

    int video_stream_index;
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    AVPacket* av_packet;
    AVFrame* av_frame;
    struct SwsContext* sws_context;
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
    (*video)->sws_context = NULL;

    AVPacket* av_packet = av_packet_alloc();
    AVFrame* av_frame = av_frame_alloc();

    if (!av_packet || !av_frame) {
        av_packet_free(&av_packet);
        av_frame_free(&av_frame);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    (*video)->av_packet = av_packet;
    (*video)->av_frame = av_frame;

    return TPXL_OK;
}

TpxlResult tpxl_get_video_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height) {

    if (!video || !width || !height) {
        return TPXL_INVALID_ARGUMENT;
    }

    *width = video->width;
    *height = video->height;

    return TPXL_OK;
}

static TpxlResult tpxl_convert_frame(struct SwsContext* sws_context, AVFrame* av_frame, TpxlImage* frame) {
    
    int result = 0;

    frame->width = av_frame->width;
    frame->height = av_frame->height;

    size_t size = frame->width * frame->height * 4;

    uint8_t* pixels = malloc(size);
    
    if (!pixels) {
        return TPXL_OUT_OF_MEMORY;
    }

    uint8_t* dst_data[4] = {0};
    int dst_linesize[4] = {0};

    result = av_image_fill_arrays(dst_data, dst_linesize, pixels, AV_PIX_FMT_RGBA, frame->width, frame->height, 1);

    if (result < 0) {
        free(pixels);
        return TPXL_VIDEO_DECODE_FAILED;
    }

    // convert to RGBA
    result = sws_scale(
        sws_context, 
        (const uint8_t* const*)av_frame->data, 
        av_frame->linesize, 
        0, 
        av_frame->height, 
        dst_data, 
        dst_linesize
    );

    if (result != (int)frame->height) {
        free(pixels);
        return TPXL_VIDEO_DECODE_FAILED;
    }

    frame->format = TPXL_FORMAT_RGBA;
    frame->pixels = pixels;

    return TPXL_OK;
}

TpxlResult tpxl_decode_video_frame(TpxlVideo* video, TpxlImage* frame) {

    if (!video || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!video->format_context || !video->codec_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    bool need_packet = true; 

    while (need_packet) {

        int result = 0;

        result = av_read_frame(video->format_context, video->av_packet);

        if (result == AVERROR_EOF) {
            // no more packets to read from file 
            av_packet_unref(video->av_packet);
            return TPXL_EOF;
        }

        if (result < 0) {
            av_packet_unref(video->av_packet);
            return TPXL_VIDEO_DECODE_FAILED;
        }

        if (video->av_packet->stream_index != video->video_stream_index) {
            av_packet_unref(video->av_packet);
            continue;
        }

        // send packet to the decoder
        result = avcodec_send_packet(video->codec_context, video->av_packet);

        if (result < 0) {
            av_packet_unref(video->av_packet);

            return TPXL_VIDEO_DECODE_FAILED;
        }

        av_packet_unref(video->av_packet);

        result = avcodec_receive_frame(video->codec_context, video->av_frame);
        
        if (result == 0) {
            // got frame
            need_packet = false;

            if (!video->sws_context) {

                struct SwsContext* sws_ctx = sws_getContext(
                    video->av_frame->width, 
                    video->av_frame->height, 
                    video->av_frame->format, 
                    video->av_frame->width, 
                    video->av_frame->height, 
                    AV_PIX_FMT_RGBA, 
                    SWS_BILINEAR, 
                    NULL, 
                    NULL, 
                    NULL
                );

                if (!sws_ctx) {
                    return TPXL_VIDEO_DECODE_FAILED; 
                }
                video->sws_context = sws_ctx;
            }

            TpxlResult tpxl_result;
            tpxl_result = tpxl_convert_frame(video->sws_context, video->av_frame, frame);

            if (tpxl_result != TPXL_OK) {
                return tpxl_result;
            }
        }
        else if (result == AVERROR(EAGAIN)) {
            // decoder needs another packet
            need_packet = true;
        }
        else if (result == AVERROR_EOF) {
            // decoder reached end of the output
            return TPXL_EOF;
        }
        else {
            // error
            return TPXL_VIDEO_DECODE_FAILED;
        }
    }

    return TPXL_OK;
}

void tpxl_close_video(TpxlVideo* video) {

    if (!video) {
        return;
    }

    av_frame_free(&video->av_frame);
    av_packet_free(&video->av_packet);

    avcodec_free_context(&video->codec_context);
    avformat_close_input(&video->format_context);
    sws_freeContext(video->sws_context);

    free(video);
}
