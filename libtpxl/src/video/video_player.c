#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <threads.h>
#include <unistd.h>

#include <libavcodec/packet.h>
#include <libavformat/avformat.h>

#include "internal/thread.h"
#include "tpxl/audio.h"
#include "tpxl/image.h"
#include "tpxl/renderer.h"
#include "tpxl/type.h"
#include "tpxl/video.h"
#include "queue/queue.h"

#include "internal/video_internal.h"
#include "internal/audio_internal.h"

static void* tpxl_demux_worker(void* args) {

    TpxlVideoPlayer* player = args;

    atomic_store(&player->demux_status, THREAD_RUNNING);

    TpxlVideo* video = player->video;
    TpxlAudio* audio = player->video->audio;

    while (!atomic_load(&player->shutdown)) {

        AVPacket* packet = av_packet_alloc();

        if (!packet) {
            tpxl_packet_queue_close(&player->video_packet_queue);
            tpxl_packet_queue_close(&player->audio_packet_queue);
            atomic_store(&player->demux_status, THREAD_ERROR);
            break;
        }

        int ret = av_read_frame(video->format_context, packet);
        
        if (ret == AVERROR_EOF) {
            av_packet_free(&packet);
            tpxl_packet_queue_close(&player->video_packet_queue);
            tpxl_packet_queue_close(&player->audio_packet_queue);
            atomic_store(&player->demux_status, THREAD_FINISHED);
            break;
        }

        if (ret < 0) {
            av_packet_free(&packet);
            tpxl_packet_queue_close(&player->video_packet_queue);
            tpxl_packet_queue_close(&player->audio_packet_queue);
            atomic_store(&player->demux_status, THREAD_ERROR);
            break;
        }

        if (packet->stream_index == video->video_stream_index) {

            TpxlResult result = tpxl_packet_queue_push(&player->video_packet_queue, packet, &player->shutdown);

            if (result != TPXL_OK) {
                av_packet_free(&packet);
                atomic_store(&player->demux_status, THREAD_ERROR);
                break;
            }
        }
        else if (packet->stream_index == audio->audio_stream_index) {

            TpxlResult result = tpxl_packet_queue_push(&player->audio_packet_queue, packet, &player->shutdown);

            if (result != TPXL_OK) {
                av_packet_free(&packet);
                atomic_store(&player->demux_status, THREAD_ERROR);
                break;
            }
        }
        else {
            av_packet_free(&packet);
        }
    }

    return NULL;
}

static void* tpxl_video_decode_worker(void* arg) {

    TpxlVideoPlayer* player = (TpxlVideoPlayer*)arg; 

    player->decode_status = THREAD_RUNNING;

    bool draining = false;

    while (!atomic_load(&player->shutdown)) {

        AVPacket* packet = NULL;

        TpxlResult result = TPXL_OK;

        if (!draining) {
            result = tpxl_packet_queue_pop(&player->video_packet_queue, &packet, &player->shutdown);

            if (result == TPXL_SHUTDOWN) {
                player->decode_status = THREAD_FINISHED;
                break;
            }
            if (result == TPXL_QUEUE_CLOSED) {

                if (atomic_load(&player->demux_status) == THREAD_FINISHED) {
                    draining = true;
                    packet = NULL;
                } 
                else {
                    tpxl_frame_queue_close(&player->frame_queue);
                    player->decode_status = THREAD_ERROR;
                    break;
                }
            } 
            else if (result != TPXL_OK) {
                tpxl_frame_queue_close(&player->frame_queue);
                player->decode_status = THREAD_ERROR;
                break;
            }
        }

        TpxlImage frame = {0};
        result = tpxl_decode_video_packet(player->video, packet, &frame);

        av_packet_free(&packet);

        if (result == TPXL_EOF) {
            tpxl_frame_queue_close(&player->frame_queue);
            player->decode_status = THREAD_FINISHED;
            break;
        }

        if (result == TPXL_VIDEO_NEED_PACKET) {
            continue;
        }

        if (result != TPXL_OK) {
            tpxl_frame_queue_close(&player->frame_queue);
            player->decode_status = THREAD_ERROR;
            break;
        }

        result = tpxl_frame_queue_push(&player->frame_queue, &frame, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            tpxl_free_frame(&frame);
            player->decode_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_free_frame(&frame);
            tpxl_frame_queue_close(&player->frame_queue);
            player->decode_status = THREAD_ERROR;
            break;
        }
    }

    return NULL;
}

static void* tpxl_upload_worker1(void* arg) {

    TpxlVideoPlayer* player = (TpxlVideoPlayer*)arg;

    player->upload1_status = THREAD_RUNNING;

    while (!atomic_load(&player->shutdown)) {

        TpxlResult result = TPXL_OK;

        TpxlImage frame = {0};
        result = tpxl_frame_queue_pop(&player->frame_queue, &frame, &player->shutdown);

        if (result == TPXL_QUEUE_CLOSED) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload1_status = THREAD_FINISHED;
            break;
        }

        if (result == TPXL_SHUTDOWN) {
            player->upload1_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_free_frame(&frame);
            player->upload1_status = THREAD_ERROR;
            break;
        }

        uint32_t frame_id = atomic_fetch_add(&player->frame_id, 1);

        result = tpxl_renderer_upload(player->renderer, &frame, frame_id);
        tpxl_free_frame(&frame);

        if (result != TPXL_OK) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload1_status = THREAD_ERROR;
            break;
        }    

        result = tpxl_frame_id_queue_push(&player->id_queue, frame_id, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            player->upload1_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload1_status = THREAD_ERROR;
            break;
        }
    }

    return NULL;
}

static void* tpxl_upload_worker2(void* arg) {

    TpxlVideoPlayer* player = (TpxlVideoPlayer*)arg;

    player->upload2_status = THREAD_RUNNING;

    while (!atomic_load(&player->shutdown)) {

        TpxlResult result = TPXL_OK;

        TpxlImage frame = {0};
        result = tpxl_frame_queue_pop(&player->frame_queue, &frame, &player->shutdown);

        if (result == TPXL_QUEUE_CLOSED) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload2_status = THREAD_FINISHED;
            break;
        }

        if (result == TPXL_SHUTDOWN) {
            player->upload2_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_free_frame(&frame);
            player->upload2_status = THREAD_ERROR;
            break;
        }

        uint32_t frame_id = atomic_fetch_add(&player->frame_id, 1);

        result = tpxl_renderer_upload(player->renderer, &frame, frame_id);
        tpxl_free_frame(&frame);

        if (result != TPXL_OK) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload2_status = THREAD_ERROR;
            break;
        }

        result = tpxl_frame_id_queue_push(&player->id_queue, frame_id, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            player->upload2_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload2_status = THREAD_ERROR;
            break;
        }        
    }

    return NULL;
}

static void tpxl_abort_video_player_creation(TpxlVideoPlayer* player) {

    if (!player) {
        return;
    }

    atomic_store(&player->shutdown, true);

    tpxl_frame_queue_close(&player->frame_queue);
    tpxl_frame_id_queue_close(&player->id_queue);

    tpxl_close_audio_player(player->audio_player);

    if (player->demux_thread_created) {
        pthread_join(player->demux_thread, NULL);
    }

    if (player->decode_thread_created) {
        pthread_join(player->decode_thread, NULL);
    }

    if (player->upload1_thread_created) {
        pthread_join(player->upload_thread1, NULL);
    }

    if (player->upload2_thread_created) {
        pthread_join(player->upload_thread2, NULL);
    }
    
    tpxl_destroy_frame_queue(&player->frame_queue);
    tpxl_destroy_frame_id_queue(&player->id_queue);

    free(player);
}

TpxlResult tpxl_create_video_player(TpxlVideoPlayer** player, TpxlRenderer* renderer, TpxlVideo* video) {

    if (!video || !renderer || !player) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = calloc(1, sizeof(TpxlVideoPlayer));

    if (!*player) {
        return TPXL_OUT_OF_MEMORY;
    }

    (*player)->renderer = renderer;
    (*player)->video = video;
    (*player)->id_queue = (TpxlFrameIDQueue){0};
    (*player)->frame_queue = (TpxlFrameQueue){0};
    (*player)->playing = true;
    
    uint32_t count = 0;
    tpxl_get_video_frame_count(video, &count);
    (*player)->frame_count = count;

    atomic_init(&(*player)->frame_id, 1);
    atomic_init(&(*player)->shutdown, false);
    atomic_init(&(*player)->demux_status, THREAD_STATUS_UNKNOWN);
    
    if (tpxl_init_frame_queue(&(*player)->frame_queue) != TPXL_OK) {
        free(*player);
        *player = NULL;
        return TPXL_VIDEO_PLAYER_CREATION_FAILED;
    }

    if (tpxl_init_frame_id_queue(&(*player)->id_queue) != TPXL_OK) {
        tpxl_destroy_frame_queue(&(*player)->frame_queue);
        free(*player);
        *player = NULL;
        return TPXL_VIDEO_PLAYER_CREATION_FAILED;
    }

    if (tpxl_init_video_audio_player(*player, video->audio) != TPXL_OK) {
        tpxl_destroy_frame_queue(&(*player)->frame_queue);
        tpxl_destroy_frame_id_queue(&(*player)->id_queue);
        free(*player);
        *player = NULL;
        return TPXL_VIDEO_PLAYER_CREATION_FAILED;
    }

    int result = 0;
    result = pthread_create(&(*player)->demux_thread, NULL, tpxl_demux_worker, *player);

    if (result != 0) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_CREATION_ERROR;
    }

    (*player)->demux_thread_created = true;

    result = pthread_create(&(*player)->decode_thread, NULL, tpxl_video_decode_worker, *player);

    if (result != 0) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_CREATION_ERROR;
    }

    (*player)->decode_thread_created = true;

    result = pthread_create(&(*player)->upload_thread1, NULL, tpxl_upload_worker1, *player);

    if (result != 0) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_CREATION_ERROR;
    }

    (*player)->upload1_thread_created = true;

    result = pthread_create(&(*player)->upload_thread2, NULL, tpxl_upload_worker2, *player);

    if (result != 0) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_CREATION_ERROR;
    }

    (*player)->upload2_thread_created = true;

    return TPXL_OK;
}

TpxlResult tpxl_update_video_player(TpxlVideoPlayer* player, TpxlRenderer* renderer) {

    if (!player || !renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    uint32_t frame_id = 0;
    TpxlResult result = tpxl_frame_id_queue_pop(&player->id_queue, &frame_id, &player->shutdown);

    if (result == TPXL_QUEUE_CLOSED && (player->upload1_status == THREAD_ERROR || player->upload2_status == THREAD_ERROR)) {
        player->playing = false;
        return TPXL_VIDEO_PLAYING_FAILED;
    }

    if (result == TPXL_QUEUE_CLOSED) {
        player->playing = false;
        return TPXL_EOF;
    }

    if (result != TPXL_OK) {
        player->playing = false;
        return result;
    }

    result = tpxl_renderer_display(renderer, frame_id);

    if (result != TPXL_OK) {
        player->playing = false;
        return result;
    }

    return TPXL_OK;
}

bool tpxl_video_playing(TpxlVideoPlayer* player) {

    if (!player) {
        return false;
    }

    return player->playing;
}

void tpxl_close_video_player(TpxlVideoPlayer* player) {

    if (!player) {
        return;
    }

    atomic_store(&player->shutdown, true);

    tpxl_frame_queue_close(&player->frame_queue);
    tpxl_frame_id_queue_close(&player->id_queue);

    tpxl_close_audio_player(player->audio_player);

    pthread_join(player->demux_thread, NULL);
    pthread_join(player->decode_thread, NULL);
    pthread_join(player->upload_thread1, NULL);
    pthread_join(player->upload_thread2, NULL);

    tpxl_destroy_frame_queue(&player->frame_queue);
    tpxl_destroy_frame_id_queue(&player->id_queue);

    free(player);
}
