#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdatomic.h>

#include <miniaudio/miniaudio.h>

#include "internal/thread.h"
#include "tpxl/audio.h"
#include "tpxl/type.h"
#include "tpxl/video.h"

#include "queue/queue.h"
#include "internal/audio_internal.h"
#include "internal/video_internal.h"

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2

static void* tpxl_audio_decode_worker(void* arg) {

    TpxlAudioPlayer* player = (TpxlAudioPlayer*)arg; 

    player->decode_status = TPXL_THREAD_RUNNING;

    while (!atomic_load(&player->shutdown)) {

        TpxlAudioFrame frame = {0};
        TpxlResult result = tpxl_decode_audio_frame(player->audio, &frame);

        if (result == TPXL_EOF) {
            tpxl_audio_frame_queue_close(&player->frame_queue);
            player->decode_status = TPXL_THREAD_FINISHED;
            break;
        }
        if (result != TPXL_OK) {
            tpxl_audio_frame_queue_close(&player->frame_queue);
            player->decode_status = TPXL_THREAD_ERROR;
            break;
        }

        result = tpxl_audio_frame_queue_push(&player->frame_queue, &frame, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            tpxl_free_audio_frame(&frame);
            player->decode_status = TPXL_THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_free_audio_frame(&frame);
            tpxl_audio_frame_queue_close(&player->frame_queue);
            player->decode_status = TPXL_THREAD_ERROR;
            break;
        }
    }

    return NULL;
}

static void* tpxl_video_audio_decode_worker(void* arg) {

    TpxlVideoPlayer* video_player = (TpxlVideoPlayer*)arg; 
    TpxlAudioPlayer* audio_player = video_player->audio_player;

    audio_player->decode_status = TPXL_THREAD_RUNNING;

    bool draining = false;

    while (!atomic_load(&audio_player->shutdown)) {

        AVPacket* packet = NULL;

        TpxlResult result = TPXL_OK;

        if (!draining) {
            result = tpxl_packet_queue_pop(&video_player->audio_packet_queue, &packet, &audio_player->shutdown);

            if (result == TPXL_SHUTDOWN) {
                audio_player->decode_status = TPXL_THREAD_FINISHED;
                break;
            }
            if (result == TPXL_QUEUE_CLOSED) {

                if (atomic_load(&video_player->demux_status) == TPXL_THREAD_FINISHED) {
                    draining = true;
                    packet = NULL;
                } 
                else {
                    tpxl_audio_frame_queue_close(&audio_player->frame_queue);
                    audio_player->decode_status = TPXL_THREAD_ERROR;
                    break;
                }
            } 
            else if (result != TPXL_OK) {
                tpxl_audio_frame_queue_close(&audio_player->frame_queue);
                audio_player->decode_status = TPXL_THREAD_ERROR;
                break;
            }
        }

        TpxlAudioFrame frame = {0};
        result = tpxl_decode_audio_packet(audio_player->audio, packet, &frame);

        av_packet_free(&packet);

        if (result == TPXL_EOF) {
            tpxl_audio_frame_queue_close(&audio_player->frame_queue);
            audio_player->decode_status = TPXL_THREAD_FINISHED;
            break;
        }

        if (result == TPXL_AUDIO_NEED_PACKET) {
            continue;
        }

        if (result != TPXL_OK) {
            tpxl_audio_frame_queue_close(&audio_player->frame_queue);
            audio_player->decode_status = TPXL_THREAD_ERROR;
            break;
        }

        result = tpxl_audio_frame_queue_push(&audio_player->frame_queue, &frame, &audio_player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            tpxl_free_audio_frame(&frame);
            audio_player->decode_status = TPXL_THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_free_audio_frame(&frame);
            tpxl_audio_frame_queue_close(&audio_player->frame_queue);
            audio_player->decode_status = TPXL_THREAD_ERROR;
            break;
        }
    }

    return NULL;
}

static void audio_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {

    (void)input;

    TpxlAudioPlayer* player = (TpxlAudioPlayer*)device->pUserData;

    float* out = (float*)output;

    ma_uint32 frames_written = 0;

    while (frames_written < frame_count) {

        if (player->current_frame.samples == NULL || player->current_frame_position >= player->current_frame.sample_count) {
             
            // Needs a decoded frame.

            TpxlResult result = tpxl_audio_frame_queue_try_pop(&player->frame_queue, &player->current_frame, &player->shutdown);

            if (result == TPXL_QUEUE_EMPTY) {
                memset(out + frames_written * AUDIO_CHANNELS, 0, (frame_count - frames_written) * AUDIO_CHANNELS * sizeof(float));
                break;
            }

            if (result == TPXL_QUEUE_CLOSED) {

                // No more audio. Fill the rest with silence.
                memset(out + frames_written * AUDIO_CHANNELS, 0, (frame_count - frames_written) * AUDIO_CHANNELS * sizeof(float));

                atomic_store(&player->playing, false);
                break;
            }

            if (result != TPXL_OK) {
                memset(out + frames_written * AUDIO_CHANNELS, 0, (frame_count - frames_written) * AUDIO_CHANNELS * sizeof(float));
                break;
            }

            player->current_frame_position = 0;
        }
        
        ma_uint32 available = player->current_frame.sample_count - player->current_frame_position;

        ma_uint32 needed = frame_count - frames_written;

        ma_uint32 count = available < needed ? available : needed;

        float* src = (float*)player->current_frame.samples + player->current_frame_position * AUDIO_CHANNELS;

        memcpy(out + frames_written * AUDIO_CHANNELS, src,  count * AUDIO_CHANNELS * sizeof(float));

        player->current_frame_position += count;
        frames_written += count;

        if (player->current_frame_position >= player->current_frame.sample_count) {

            // The frame is completely consumed.

            tpxl_free_audio_frame(&player->current_frame);
            player->current_frame_position = 0;
        }
    }

    atomic_fetch_add_explicit(&player->frames_submitted, frames_written, memory_order_relaxed);
}

TpxlResult tpxl_create_audio_player(TpxlAudioPlayer** player, TpxlAudio* audio) {

    if (!player || !audio) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = calloc(1, sizeof(TpxlAudioPlayer));

    if (!*player) {
        return TPXL_OUT_OF_MEMORY;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.playback.channels = AUDIO_CHANNELS;
    config.pUserData = *player;
    config.dataCallback = audio_callback;
    config.periodSizeInFrames = 480;
    config.noPreSilencedOutputBuffer = true;

    ma_result result = ma_device_init(NULL, &config, &(*player)->device);

    if (result != MA_SUCCESS) {
        free(*player);
        *player = NULL;
        return TPXL_AUDIO_PLAYER_CREATION_FAILED;
    }

    (*player)->audio = audio;

    atomic_init(&(*player)->playing, false);
    atomic_init(&(*player)->shutdown, false);
    atomic_init(&(*player)->frames_submitted, 0);

    if (tpxl_init_audio_frame_queue(&(*player)->frame_queue) != TPXL_OK) {
        ma_device_uninit(&(*player)->device);
        free(*player);
        *player = NULL;
        return TPXL_AUDIO_PLAYER_CREATION_FAILED;
    }

    int thread_result = 0;
    thread_result = pthread_create(&(*player)->decode_thread, NULL, tpxl_audio_decode_worker, *player);

    if (thread_result != 0) {
        ma_device_uninit(&(*player)->device);
        tpxl_destroy_audio_frame_queue(&(*player)->frame_queue);
        free(*player);
        *player = NULL;
        return TPXL_THREAD_CREATION_ERROR;
    }

    return TPXL_OK;
}

TpxlResult tpxl_init_video_audio_player(TpxlVideoPlayer* video_player, TpxlAudio* audio) {

    if (!video_player || !audio) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlAudioPlayer* player = NULL;
    player = calloc(1, sizeof(TpxlAudioPlayer));

    if (!player) {
        return TPXL_OUT_OF_MEMORY;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.playback.channels = AUDIO_CHANNELS;
    config.pUserData = player;
    config.dataCallback = audio_callback;
    config.periodSizeInFrames = 480;
    config.noPreSilencedOutputBuffer = true;

    ma_result result = ma_device_init(NULL, &config, &(player)->device);

    if (result != MA_SUCCESS) {
        free(player);
        player = NULL;
        return TPXL_AUDIO_PLAYER_CREATION_FAILED;
    }

    (player)->audio = audio;

    atomic_init(&(player)->playing, false);
    atomic_init(&(player)->shutdown, false);
    atomic_init(&player->frames_submitted, 0);

    if (tpxl_init_audio_frame_queue(&(player)->frame_queue) != TPXL_OK) {
        ma_device_uninit(&(player)->device);
        free(player);
        player = NULL;
        return TPXL_AUDIO_PLAYER_CREATION_FAILED;
    }

    int thread_result = 0;
    thread_result = pthread_create(&(player)->decode_thread, NULL, tpxl_video_audio_decode_worker, video_player);

    if (thread_result != 0) {
        ma_device_uninit(&(player)->device);
        tpxl_destroy_audio_frame_queue(&(player)->frame_queue);
        free(player);
        player = NULL;
        return TPXL_THREAD_CREATION_ERROR;
    }

    video_player->audio_player = player;

    return TPXL_OK;
}

TpxlResult tpxl_play_audio(TpxlAudioPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    ma_result result = ma_device_start(&player->device);

    if (result != MA_SUCCESS) {
        return TPXL_AUDIO_PLAYING_FAILED;
    }

    atomic_store(&player->playing, true);

    return TPXL_OK;
}

double tpxl_get_audio_clock(TpxlAudioPlayer* player) {
    
    assert(player);

    return (double)atomic_load_explicit(&player->frames_submitted, memory_order_relaxed) / AUDIO_SAMPLE_RATE;
}

bool tpxl_audio_playing(TpxlAudioPlayer* player) {

    if (!player) {
        return false;
    }

    return atomic_load(&player->playing);
}

void tpxl_close_audio_player(TpxlAudioPlayer* player) {

    if (!player) {
        return;
    }

    atomic_store(&player->shutdown, true);
    atomic_store(&player->playing, false);

    tpxl_audio_frame_queue_close(&player->frame_queue);

    pthread_join(player->decode_thread, NULL);

    tpxl_destroy_audio_frame_queue(&player->frame_queue);

    ma_device_uninit(&player->device);

    free(player);
}
