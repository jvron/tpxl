
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/rational.h>

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
