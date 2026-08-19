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
#include "internal/video_internal.h"


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
    int video_stream_index = -1;

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
    (*video)->output_width = video_stream->codecpar->width;
    (*video)->output_height = video_stream->codecpar->height;
    (*video)->format = TPXL_FORMAT_RGB;
    (*video)->video_stream_format = video_stream->codecpar->format;
    (*video)->duration = video_stream->duration;
    (*video)->frame_count = video_stream->nb_frames;
    (*video)->video_stream_index = video_stream_index;
    (*video)->format_context = format_context;
    (*video)->codec_context = codec_context;
    (*video)->draining = false;

    struct SwsContext* sws_ctx = sws_getContext(
        video_stream->codecpar->width, 
        video_stream->codecpar->height,
        video_stream->codecpar->format, 
        video_stream->codecpar->width, 
        video_stream->codecpar->height, 
        AV_PIX_FMT_RGB24, 
        SWS_BILINEAR,
        NULL, 
        NULL, 
        NULL
    );

    if (!sws_ctx) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*video);
        return TPXL_VIDEO_LOAD_FAILED; 
    }

    AVPacket* av_packet = av_packet_alloc();
    AVFrame* av_frame = av_frame_alloc();

    if (!av_packet || !av_frame) {
        sws_freeContext(sws_ctx);
        av_packet_free(&av_packet);
        av_frame_free(&av_frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*video);
        return TPXL_VIDEO_LOAD_FAILED;
    }

    (*video)->sws_context = sws_ctx;
    (*video)->av_packet = av_packet;
    (*video)->av_frame = av_frame;

    return TPXL_OK;
}

TpxlResult tpxl_video_set_output_size(TpxlVideo* video, uint32_t width, uint32_t height) {

    if (!video || !width || !height) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (video->output_width == width && video->output_height == height) {
        return TPXL_OK;
    }
    
    struct SwsContext* sws_ctx = sws_getContext(
        video->width, 
        video->height,
        video->video_stream_format, 
        width,
        height,
        AV_PIX_FMT_RGB24, 
        SWS_BILINEAR,
        NULL, 
        NULL, 
        NULL
    );

    if (!sws_ctx) {
        return TPXL_ERROR;
    }

    sws_freeContext(video->sws_context);
    video->output_width = width;
    video->output_height = height;
    video->sws_context = sws_ctx;

    return TPXL_OK;
}

TpxlResult tpxl_get_video_source_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height) {

    if (!video || !width || !height) {
        return TPXL_INVALID_ARGUMENT;
    }

    *width = video->width;
    *height = video->height;

    return TPXL_OK;
}

TpxlResult tpxl_get_video_output_dimensions(TpxlVideo* video, uint32_t* width, uint32_t* height) {

    if (!video || !width || !height) {
        return TPXL_INVALID_ARGUMENT;
    }

    *width = video->output_width;
    *height = video->output_height;

    return TPXL_OK;
}

TpxlResult tpxl_get_video_format(TpxlVideo* video, TpxlFormat* format) {

    if (!video || !format) {
        return TPXL_INVALID_ARGUMENT;
    }

    *format = video->format;

    return TPXL_OK;
}

TpxlResult tpxl_get_video_frame_count(TpxlVideo* video, uint32_t* frame_count) {

    if (!video || !frame_count) {
        return TPXL_INVALID_ARGUMENT;
    }

    *frame_count = video->frame_count;

    return TPXL_OK;
}

static TpxlResult tpxl_convert_frame(struct SwsContext* sws_context, AVFrame* av_frame, uint32_t output_width, uint32_t output_height, TpxlImage* frame) {
    
    int result = 0;

    frame->width = output_width;
    frame->height = output_height;
    frame->format = TPXL_FORMAT_RGB;

    size_t size = frame->width * frame->height * tpxl_format_to_channels(frame->format);

    uint8_t* pixels = malloc(size);
    
    if (!pixels) {
        return TPXL_OUT_OF_MEMORY;
    }

    uint8_t* dst_data[4] = {0};
    int dst_linesize[4] = {0};

    result = av_image_fill_arrays(dst_data, dst_linesize, pixels, AV_PIX_FMT_RGB24, frame->width, frame->height, 1);

    if (result < 0) {
        free(pixels);
        return TPXL_VIDEO_DECODE_FAILED;
    }

    // convert to RGB
    result = sws_scale(
        sws_context, 
        (const uint8_t* const*)av_frame->data, 
        av_frame->linesize, 
        0, 
        av_frame->height, 
        dst_data, 
        dst_linesize
    );

    if (result != (int)output_height) {
        free(pixels);
        return TPXL_VIDEO_DECODE_FAILED;
    }

    frame->pixels = pixels;

    return TPXL_OK;
}

static TpxlResult tpxl_receive_frame(struct SwsContext* sws_context, AVCodecContext* codec_context, AVFrame* av_frame, uint32_t output_width, uint32_t output_height, TpxlImage* frame) {

    int result = 0;
    // process frame
    result = avcodec_receive_frame(codec_context, av_frame);
    
    if (result == 0) {
        // got frame
        
        TpxlResult tpxl_result;
        tpxl_result = tpxl_convert_frame(sws_context, av_frame, output_width, output_height, frame);

        if (tpxl_result != TPXL_OK) {
            return tpxl_result;
        }

        return TPXL_OK;
    }
    if (result == AVERROR(EAGAIN)) {
        // decoder needs another packet
        return TPXL_VIDEO_NEED_PACKET;
    }
    if (result == AVERROR_EOF) {
        // decoder has been fully drained; no more frames will be produced.
        return TPXL_EOF;
    }

    // error
    return TPXL_VIDEO_DECODE_FAILED;
}

TpxlResult tpxl_decode_video_frame(TpxlVideo* video, TpxlImage* out_frame) {

    if (!video || !out_frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!video->format_context || !video->codec_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult frame_result = TPXL_OK;

    // try to receive pending frames
    frame_result = tpxl_receive_frame(
        video->sws_context, 
        video->codec_context, 
        video->av_frame, 
        video->output_width, 
        video->output_height,
        out_frame
    );

    if (frame_result != TPXL_VIDEO_NEED_PACKET) {
        return frame_result;
    }

    while (true) {

        int result = 0;

        if (!video->draining) {

            result = av_read_frame(video->format_context, video->av_packet);
    
            if (result == AVERROR_EOF) {
                // no more packets to read from file 
                // drain decoder
                video->draining = true;
                result = avcodec_send_packet(video->codec_context, NULL);

                if (result < 0) {
                    av_packet_unref(video->av_packet);
                    return TPXL_VIDEO_DECODE_FAILED;
                }
                continue;
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
        }

        frame_result = tpxl_receive_frame(
            video->sws_context,
            video->codec_context,
            video->av_frame,
            video->output_width,
            video->output_height,
            out_frame
        );

        if (frame_result != TPXL_VIDEO_NEED_PACKET) {
            return frame_result;
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
