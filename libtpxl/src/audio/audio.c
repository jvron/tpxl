#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>

#include "tpxl/audio.h"
#include "tpxl/type.h"

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

    bool draining;
};

TpxlResult tpxl_open_audio(const char* path, TpxlAudio** audio) {

    if (!path || !audio) {
        return TPXL_INVALID_ARGUMENT;
    }

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

    if (audio_stream_index == -1) {
        avformat_close_input(&format_context);
        return TPXL_AUDIO_LOAD_FAILED; 
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

    *audio = malloc(sizeof(TpxlAudio));
    
    if (!*audio) {
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return TPXL_OUT_OF_MEMORY;
    }

    (*audio)->audio_stream_index = audio_stream_index;
    (*audio)->time_base = audio_stream->time_base;
    (*audio)->sample_rate = codec_context->sample_rate;
    (*audio)->channel_layout = codec_context->ch_layout;
    (*audio)->format_context = format_context;
    (*audio)->codec_context = codec_context;

    AVPacket* av_packet = av_packet_alloc();
    AVFrame* av_frame = av_frame_alloc();

    if (!av_packet || !av_frame) {
        av_packet_free(&av_packet);
        av_frame_free(&av_frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        free(*audio);
        return TPXL_OUT_OF_MEMORY;
    }

    (*audio)->av_packet = av_packet;
    (*audio)->av_frame = av_frame;

    return TPXL_OK;
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

static TpxlResult tpxl_receive_audio_frame(AVCodecContext* codec_context, AVFrame* av_frame, TpxlAudioFrame* frame) {

    int result = 0;
    result = avcodec_receive_frame(codec_context, av_frame);
    
    if (result == 0) {
        // got frame

        int size = av_samples_get_buffer_size(
            NULL, 
            av_frame->ch_layout.nb_channels,
            av_frame->nb_samples,
            av_frame->format,
            1
        );

        if (size < 0) {
            return TPXL_AUDIO_DECODE_FAILED;
        }

        uint8_t* samples = malloc((size_t)size);

        if (!samples) {
            return TPXL_OUT_OF_MEMORY;
        }

        uint8_t* dst_data[AV_NUM_DATA_POINTERS] = {0};

        result = av_samples_fill_arrays(dst_data,
            NULL,
            samples, 
            av_frame->ch_layout.nb_channels,
            av_frame->nb_samples,
            av_frame->format, 
            1
        );

        if (result < 0) {
            free(samples);
            return TPXL_AUDIO_DECODE_FAILED;
        }

        result = av_samples_copy(
            dst_data,
            av_frame->data,
            0,
            0,
            av_frame->nb_samples,
            av_frame->ch_layout.nb_channels,
            av_frame->format
        );

        if (result < 0) {
            free(samples);
            return TPXL_AUDIO_DECODE_FAILED;
        }

        frame->samples = samples;
        frame->format = tpxl_audio_format_map[av_frame->format];
        frame->pts = av_frame->pts;
        frame->sample_count = av_frame->nb_samples;

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
        audio->codec_context, 
        audio->av_frame,
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
            audio->codec_context, 
            audio->av_frame,
            out_audio_frame
        );

        if (frame_result != TPXL_AUDIO_NEED_PACKET) {
            return frame_result;
        }
    }

    return TPXL_OK;
}

void tpxl_close_audio(TpxlAudio* audio) {

    if (!audio) {
        return;
    }

    av_packet_free(&audio->av_packet);
    av_frame_free(&audio->av_frame);

    avcodec_free_context(&audio->codec_context);
    avformat_close_input(&audio->format_context);

    free(audio);
}
