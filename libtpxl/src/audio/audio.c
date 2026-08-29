#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>

#include "tpxl/audio.h"
#include "tpxl/type.h"

#include "internal/audio_internal.h"

#define AUDIO_SAMPLE_RATE 48000

TpxlResult tpxl_open_audio(const char* path, TpxlAudio** audio) {

    if (!path || !audio) {
        return TPXL_INVALID_ARGUMENT;
    }

    av_log_set_level(AV_LOG_ERROR);
    
    *audio = NULL;

    AVFormatContext* format_context = NULL;

    if (avformat_open_input(&format_context, path, NULL, NULL) < 0) {
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Read stream information.
    if (avformat_find_stream_info(format_context, NULL) < 0) {
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    AVStream* audio_stream = NULL;
    int audio_stream_index = -1;

    for (size_t i = 0; i < format_context->nb_streams; i++) {

        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            audio_stream = format_context->streams[i];
            break;
        } 
    }

    if (!audio_stream) {
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Find the decoder for the audio stream.
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);

    if (!codec) {
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Create and configure the decoder context.
    AVCodecContext* codec_context = avcodec_alloc_context3(codec);

    if (!codec_context) {
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Copy the stream's codec parameters into the decoder context
    if (avcodec_parameters_to_context(codec_context, audio_stream->codecpar) < 0) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Open the decoder.
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    *audio = calloc(1, sizeof(TpxlAudio));
    
    if (!*audio) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_OUT_OF_MEMORY;
    }

    (*audio)->swr_context = NULL;
    (*audio)->output_sample_rate = AUDIO_SAMPLE_RATE;
    (*audio)->output_format = AV_SAMPLE_FMT_FLT;
    av_channel_layout_default(&(*audio)->output_channel_layout, 2);

    int result = swr_alloc_set_opts2(
        &(*audio)->swr_context,
        &(*audio)->output_channel_layout,
        (*audio)->output_format,
        (*audio)->output_sample_rate,
        &codec_context->ch_layout,
        codec_context->sample_fmt,
        codec_context->sample_rate,
        0,
        NULL
    );

    if (result < 0) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*audio);
        *audio = NULL;
        return TPXL_AUDIO_LOAD_FAILED;;
    }

    if (swr_init((*audio)->swr_context) < 0) {
        swr_free(&(*audio)->swr_context);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*audio);
        *audio = NULL;
        return TPXL_AUDIO_LOAD_FAILED;
    }

    if (av_channel_layout_copy(&(*audio)->channel_layout, &codec_context->ch_layout) < 0) {
        swr_free(&(*audio)->swr_context);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*audio);
        *audio = NULL;
        return TPXL_AUDIO_LOAD_FAILED;
    }

    (*audio)->audio_stream_index = audio_stream_index;
    (*audio)->time_base = audio_stream->time_base;
    (*audio)->sample_rate = codec_context->sample_rate;
    (*audio)->format_context = format_context;
    (*audio)->codec_context = codec_context;
    (*audio)->draining = false;
    (*audio)->drain_sent = false;

    AVPacket* av_packet = av_packet_alloc();
    AVFrame* av_frame = av_frame_alloc();

    if (!av_packet || !av_frame) {
        av_packet_free(&av_packet);
        av_frame_free(&av_frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*audio);
        *audio = NULL;
        return TPXL_OUT_OF_MEMORY;
    }

    (*audio)->av_packet = av_packet;
    (*audio)->av_frame = av_frame;

    return TPXL_OK;
}

TpxlResult tpxl_init_video_audio(TpxlAudio** audio, AVFormatContext* format_context) {

    if (!audio || !format_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    *audio = NULL;

    AVStream* audio_stream = NULL;
    int audio_stream_index = -1;

    for (size_t i = 0; i < format_context->nb_streams; i++) {

        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            audio_stream = format_context->streams[i];
            break;
        } 
    }

    if (!audio_stream) {
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Find the decoder for the audio stream.
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);

    if (!codec) {
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Create and configure the decoder context.
    AVCodecContext* codec_context = avcodec_alloc_context3(codec);

    if (!codec_context) {
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Copy the stream's codec parameters into the decoder context
    if (avcodec_parameters_to_context(codec_context, audio_stream->codecpar) < 0) {
        avcodec_free_context(&codec_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    // Open the decoder.
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        avcodec_free_context(&codec_context);
        return TPXL_AUDIO_LOAD_FAILED;
    }

    *audio = calloc(1, sizeof(TpxlAudio));
    
    if (!*audio) {
        avcodec_free_context(&codec_context);
        return TPXL_OUT_OF_MEMORY;
    }

    (*audio)->swr_context = NULL;
    (*audio)->output_sample_rate = AUDIO_SAMPLE_RATE;
    (*audio)->output_format = AV_SAMPLE_FMT_FLT;
    av_channel_layout_default(&(*audio)->output_channel_layout, 2);

    int result = swr_alloc_set_opts2(
        &(*audio)->swr_context,
        &(*audio)->output_channel_layout,
        (*audio)->output_format,
        (*audio)->output_sample_rate,
        &codec_context->ch_layout,
        codec_context->sample_fmt,
        codec_context->sample_rate,
        0,
        NULL
    );

    if (result < 0) {
        avcodec_free_context(&codec_context);
        free(*audio);
        *audio = NULL;
        return TPXL_AUDIO_LOAD_FAILED;;
    }

    if (swr_init((*audio)->swr_context) < 0) {
        swr_free(&(*audio)->swr_context);
        avcodec_free_context(&codec_context);
        free(*audio);
        *audio = NULL;
        return TPXL_AUDIO_LOAD_FAILED;
    }

    if (av_channel_layout_copy(&(*audio)->channel_layout, &codec_context->ch_layout) < 0) {
        swr_free(&(*audio)->swr_context);
        avcodec_free_context(&codec_context);
        free(*audio);
        *audio = NULL;
        return TPXL_AUDIO_LOAD_FAILED;
    }

    (*audio)->audio_stream_index = audio_stream_index;
    (*audio)->time_base = audio_stream->time_base;
    (*audio)->sample_rate = codec_context->sample_rate;
    (*audio)->codec_context = codec_context;
    (*audio)->draining = false;
    (*audio)->format_context = NULL;
    (*audio)->av_packet = NULL;
    (*audio)->drain_sent = false;
    
    AVFrame* av_frame = av_frame_alloc();

    if (!av_frame) {
        swr_free(&(*audio)->swr_context);
        av_frame_free(&av_frame);
        avcodec_free_context(&codec_context);
        free(*audio);
        *audio = NULL;
        return TPXL_OUT_OF_MEMORY;
    }

    (*audio)->av_frame = av_frame;

    return TPXL_OK;
}

double tpxl_get_audio_duration(TpxlAudio* audio) {

    if (!audio) {
        return 0.0;
    }

    if (audio->format_context->duration == AV_NOPTS_VALUE) {
        return 0.0;
    }

    return (double)audio->format_context->duration / AV_TIME_BASE;
}

static const TpxlAudioFormat tpxl_audio_format_map[] = {
    [AV_SAMPLE_FMT_U8]   = TPXL_AUDIO_FORMAT_U8,
    [AV_SAMPLE_FMT_S16]  = TPXL_AUDIO_FORMAT_S16,
    [AV_SAMPLE_FMT_S32]  = TPXL_AUDIO_FORMAT_S32,
    [AV_SAMPLE_FMT_S64]  = TPXL_AUDIO_FORMAT_S64,
    [AV_SAMPLE_FMT_FLT]  = TPXL_AUDIO_FORMAT_FLT,
    [AV_SAMPLE_FMT_DBL]  = TPXL_AUDIO_FORMAT_DBL,

    [AV_SAMPLE_FMT_U8P]  = TPXL_AUDIO_FORMAT_U8P,
    [AV_SAMPLE_FMT_S16P] = TPXL_AUDIO_FORMAT_S16P,
    [AV_SAMPLE_FMT_S32P] = TPXL_AUDIO_FORMAT_S32P,
    [AV_SAMPLE_FMT_S64P] = TPXL_AUDIO_FORMAT_S64P,
    [AV_SAMPLE_FMT_FLTP] = TPXL_AUDIO_FORMAT_FLTP,
    [AV_SAMPLE_FMT_DBLP] = TPXL_AUDIO_FORMAT_DBLP,
};

static TpxlResult tpxl_receive_audio_frame(TpxlAudio* audio, TpxlAudioFrame* frame) {

    if (!audio || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    AVCodecContext* codec_context = audio->codec_context;
    AVFrame* av_frame = audio->av_frame;

    int result = 0;
    result = avcodec_receive_frame(codec_context, av_frame);
    
    if (result == 0) {
        // got frame
        
        int64_t delay = swr_get_delay(audio->swr_context, codec_context->sample_rate) + av_frame->nb_samples;

        int output_sample_count = av_rescale_rnd(
            delay,
            audio->output_sample_rate,
            codec_context->sample_rate,
            AV_ROUND_UP
        );

        int output_channels = audio->output_channel_layout.nb_channels;

        int size = av_samples_get_buffer_size(
            NULL, 
            output_channels,
            output_sample_count,
            audio->output_format,
            1
        );

        if (size < 0) {
            return TPXL_AUDIO_DECODE_FAILED;
        }

        uint8_t* samples = malloc((size_t)size);

        if (!samples) {
            return TPXL_OUT_OF_MEMORY;
        }

        uint8_t* output_data[AV_NUM_DATA_POINTERS] = {0};

        result = av_samples_fill_arrays(
            output_data,
            NULL,
            samples, 
            output_channels,
            output_sample_count,
            audio->output_format, 
            1
        );

        if (result < 0) {
            free(samples);
            return TPXL_AUDIO_DECODE_FAILED;
        }

        int converted_samples = swr_convert(
            audio->swr_context,
            output_data,
            output_sample_count,
            (const uint8_t**)av_frame->data, 
            av_frame->nb_samples
        );

        if (converted_samples < 0) {
            free(samples);
            return TPXL_AUDIO_DECODE_FAILED;
        }

        frame->samples = samples;
        frame->format = tpxl_audio_format_map[audio->output_format];
        frame->sample_count = converted_samples;
        frame->pts = av_frame->pts;

        return TPXL_OK;
    }
    if (result == AVERROR(EAGAIN)) {
        // decoder needs another packet
        return TPXL_AUDIO_NEED_PACKET;
    }
    if (result == AVERROR_EOF) {
        // decoder has been fully drained; no more frames will be produced.
        return TPXL_EOF;
    }

    // error
    return TPXL_AUDIO_DECODE_FAILED;
}

TpxlResult tpxl_decode_audio_frame(TpxlAudio* audio, TpxlAudioFrame* out_audio_frame) {

    if (!audio || !out_audio_frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!audio->format_context || !audio->codec_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult frame_result = TPXL_OK;

    // try to receive pending frames
    frame_result = tpxl_receive_audio_frame(
        audio,
        out_audio_frame
    );

    if (frame_result != TPXL_AUDIO_NEED_PACKET) {
        return frame_result;
    }

    while (true) {

        int result = 0;

        if (!audio->draining) {

            result = av_read_frame(audio->format_context, audio->av_packet);
    
            if (result == AVERROR_EOF) {
                // no more packets to read from file 
                // drain decoder
                audio->draining = true;
                result = avcodec_send_packet(audio->codec_context, NULL);

                if (result < 0) {
                    av_packet_unref(audio->av_packet);
                    return TPXL_AUDIO_DECODE_FAILED;
                }
                continue;
            }
            if (result < 0) {
                av_packet_unref(audio->av_packet);
                return TPXL_AUDIO_DECODE_FAILED;
            }
            if (audio->av_packet->stream_index != audio->audio_stream_index) {
                av_packet_unref(audio->av_packet);
                continue;
            }
    
            // send packet to the decoder
            result = avcodec_send_packet(audio->codec_context, audio->av_packet);
    
            if (result < 0) {
                av_packet_unref(audio->av_packet);
                return TPXL_AUDIO_DECODE_FAILED;
            }
            av_packet_unref(audio->av_packet);
        }

        frame_result = tpxl_receive_audio_frame(
            audio,
            out_audio_frame
        );

        if (frame_result != TPXL_AUDIO_NEED_PACKET) {
            return frame_result;
        }
    }

    return TPXL_OK;
}

TpxlResult tpxl_decode_audio_packet(TpxlAudio* audio, AVPacket* packet, TpxlAudioFrame* out_audio_frame) {

    if (!audio || !out_audio_frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!audio->codec_context) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = TPXL_OK;

    // Try to receive pending frames in the decoder.
    result = tpxl_receive_audio_frame(
        audio, 
        out_audio_frame
    );

    if (result == TPXL_OK) {
        return TPXL_OK;
    }

    if (result == TPXL_EOF) {
        return TPXL_EOF;
    }

    if (result != TPXL_AUDIO_NEED_PACKET) {
        return result;
    }
    
    int ret = 0;

    if (packet) {

        // Send the packet supplied by the demuxer.
        // packet == NULL tells FFmpeg to drain the decoder.
        ret = avcodec_send_packet(audio->codec_context, packet);
    }
    else if (!packet && !audio->drain_sent) {

        // Start draining the decoder.
        ret = avcodec_send_packet(audio->codec_context, NULL);

        if (ret < 0) {

            if (ret == AVERROR_EOF) {
                return TPXL_EOF;
            }

            return TPXL_AUDIO_DECODE_FAILED;
        }

        audio->drain_sent = true;
    }

    if (ret == AVERROR_EOF) {
        return TPXL_EOF;
    }

    if (ret < 0) {
        return TPXL_AUDIO_DECODE_FAILED;
    }

    // Try to receive the frame produced by that packet.
    result = tpxl_receive_audio_frame(
        audio, 
        out_audio_frame
    );

    return result;
}

void tpxl_free_audio_frame(TpxlAudioFrame* frame) {
    
    if (!frame) {
        return;
    }

    free(frame->samples);
    *frame = (TpxlAudioFrame){0};
}

static void tpxl_destroy_audio_resources(TpxlAudio* audio) {

    if (!audio) {
        return;
    }

    av_packet_free(&audio->av_packet);
    av_frame_free(&audio->av_frame);

    swr_free(&audio->swr_context);

    avcodec_free_context(&audio->codec_context);

    av_channel_layout_uninit(&audio->channel_layout);
    av_channel_layout_uninit(&audio->output_channel_layout);
}

void tpxl_close_audio(TpxlAudio* audio) {

    if (!audio) {
        return;
    }

    tpxl_destroy_audio_resources(audio);

    avformat_close_input(&audio->format_context);

    free(audio);
}

void tpxl_close_video_audio(TpxlAudio* audio) {

    if (!audio) {
        return;
    }

    tpxl_destroy_audio_resources(audio);

    free(audio);
}
