#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

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
#include "tpxl/audio.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

#include "internal/audio_internal.h"
#include "internal/video_internal.h"

TpxlResult tpxl_open_video(const char* path, TpxlVideo** video) {

    if (!path || !video) {
        return TPXL_INVALID_ARGUMENT;
    }

    *video = NULL;

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
    (*video)->time_base = video_stream->time_base;
    (*video)->format_context = format_context;
    (*video)->codec_context = codec_context;
    (*video)->drain_sent = false;

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
        *video = NULL;
        return TPXL_VIDEO_LOAD_FAILED; 
    }

    AVFrame* av_frame = av_frame_alloc();

    if (!av_frame) {
        sws_freeContext(sws_ctx);
        av_frame_free(&av_frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*video);
        *video = NULL;
        return TPXL_VIDEO_LOAD_FAILED;
    }

    (*video)->sws_context = sws_ctx;
    (*video)->av_frame = av_frame;

    TpxlAudio* audio = NULL;
    TpxlResult result = tpxl_init_video_audio(&audio, format_context);

    if (result != TPXL_OK) {
        sws_freeContext(sws_ctx);
        av_frame_free(&av_frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*video);
        *video = NULL;
        return result;
    }

    (*video)->audio = audio;

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

uint32_t tpxl_get_video_frame_count(TpxlVideo* video) {

    assert(video);

    return video->frame_count;
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

static TpxlResult tpxl_receive_frame(struct SwsContext* sws_context, AVCodecContext* codec_context, AVFrame* av_frame, uint32_t output_width, uint32_t output_height, TpxlVideoFrame* video_frame) {

    int result = 0;

    // Process frame.
    result = avcodec_receive_frame(codec_context, av_frame);
    
    if (result == 0) {
        // Got frame.
        TpxlResult tpxl_result;
        tpxl_result = tpxl_convert_frame(sws_context, av_frame, output_width, output_height, &video_frame->frame);

        if (tpxl_result != TPXL_OK) {
            return tpxl_result;
        }
        
        video_frame->pts = av_frame->pts;

        return TPXL_OK;
    }
    if (result == AVERROR(EAGAIN)) {
        // Decoder needs another packet
        return TPXL_VIDEO_NEED_PACKET;
    }
    if (result == AVERROR_EOF) {
        // Decoder has been fully drained, no more frames will be produced.
        return TPXL_EOF;
    }

    return TPXL_VIDEO_DECODE_FAILED;
}

TpxlResult tpxl_decode_video_packet(TpxlVideo* video, AVPacket* packet, TpxlVideoFrame* out_frame) {

    if (!video || !out_frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!video->codec_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = TPXL_OK;

    // Try to receive pending frames in the decoder.
    result = tpxl_receive_frame(
        video->sws_context, 
        video->codec_context, 
        video->av_frame, 
        video->output_width, 
        video->output_height,
        out_frame
    );

    if (result == TPXL_OK) {
        return TPXL_OK;
    }

    if (result == TPXL_EOF) {
        return TPXL_EOF;
    }

    if (result != TPXL_VIDEO_NEED_PACKET) {
        return result;
    }

    int ret = 0;
    
    if (packet) {

        // Send the packet supplied by the demuxer.
        // packet == NULL tells FFmpeg to drain the decoder.
        ret = avcodec_send_packet(video->codec_context, packet);
    }
    else if (!packet && !video->drain_sent) {

        // Start draining the decoder.
        ret = avcodec_send_packet(video->codec_context, NULL);

        if (ret < 0) {

            if (ret == AVERROR_EOF) {
                return TPXL_EOF;
            }

            return TPXL_VIDEO_DECODE_FAILED;
        }

        video->drain_sent = true;
    }

    if (ret == AVERROR_EOF) {
        return TPXL_EOF;
    }
    
    if (ret < 0) {
        return TPXL_VIDEO_DECODE_FAILED;
    }

    // Try to receive the frame produced by that packet,
    // or by draining the decoder.
    result = tpxl_receive_frame(
        video->sws_context,
        video->codec_context,
        video->av_frame,
        video->output_width,
        video->output_height,
        out_frame
    );

    return result;
}

void tpxl_free_video_frame(TpxlVideoFrame* video_frame) {

    if (!video_frame) {
        return;
    }

    tpxl_free_frame(&video_frame->frame);
    *video_frame = (TpxlVideoFrame){0};
}

void tpxl_close_video(TpxlVideo* video) {

    if (!video) {
        return;
    }

    tpxl_close_video_audio(video->audio);

    av_frame_free(&video->av_frame);

    avcodec_free_context(&video->codec_context);
    avformat_close_input(&video->format_context);
    sws_freeContext(video->sws_context);

    free(video);
}
