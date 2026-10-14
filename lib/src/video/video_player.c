#include <assert.h>

#include "tpxl/type.h"
#include "tpxl/util.h"
#include "tpxl/renderer.h"

#include "util/thread.h"
#include "internal/video_internal.h"
#include "internal/audio_internal.h"

#define VIDEO_SYNC_MIN (-0.040)
#define VIDEO_SYNC_MAX (0.020)

static void* tpxl_demux_worker(void* args) {

    TpxlVideoPlayer* player = args;

    atomic_store(&player->demux_status, TPXL_THREAD_STATUS_RUNNING);

    TpxlVideo* video = player->video;
    TpxlAudio* audio = player->video->audio;

    while (!atomic_load(&player->shutdown)) {

        AVPacket* packet = av_packet_alloc();

        if (!packet) {
            tpxl_packet_queue_close(&player->video_packet_queue);
            tpxl_packet_queue_close(&player->audio_packet_queue);
            atomic_store(&player->demux_status, TPXL_THREAD_STATUS_FAILED);
            break;
        }

        int ret = av_read_frame(video->format_context, packet);
        
        if (ret == AVERROR_EOF) {
            av_packet_free(&packet);
            tpxl_packet_queue_close(&player->video_packet_queue);
            tpxl_packet_queue_close(&player->audio_packet_queue);
            atomic_store(&player->demux_status, TPXL_THREAD_STATUS_FINISHED);
            break;
        }

        if (ret < 0) {
            av_packet_free(&packet);
            tpxl_packet_queue_close(&player->video_packet_queue);
            tpxl_packet_queue_close(&player->audio_packet_queue);
            atomic_store(&player->demux_status, TPXL_THREAD_STATUS_FAILED);
            break;
        }

        if (packet->stream_index == video->video_stream_index) {

            TpxlResult result = tpxl_packet_queue_push(&player->video_packet_queue, packet, &player->shutdown);

            if (result != TPXL_OK) {
                av_packet_free(&packet);
                atomic_store(&player->demux_status, TPXL_THREAD_STATUS_FAILED);
                break;
            }
        }
        else if (packet->stream_index == audio->audio_stream_index) {

            TpxlResult result = tpxl_packet_queue_push(&player->audio_packet_queue, packet, &player->shutdown);

            if (result != TPXL_OK) {
                av_packet_free(&packet);
                atomic_store(&player->demux_status, TPXL_THREAD_STATUS_FAILED);
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

    player->decode_status = TPXL_THREAD_STATUS_RUNNING;

    bool draining = false;

    while (!atomic_load(&player->shutdown)) {

        AVPacket* packet = NULL;

        TpxlResult result = TPXL_OK;

        if (!draining) {
            result = tpxl_packet_queue_pop(&player->video_packet_queue, &packet, &player->shutdown);

            if (result == TPXL_SHUTDOWN) {
                player->decode_status = TPXL_THREAD_STATUS_FINISHED;
                break;
            }

            if (result == TPXL_QUEUE_CLOSED) {

                if (atomic_load(&player->demux_status) == TPXL_THREAD_STATUS_FINISHED) {
                    draining = true;
                    packet = NULL;
                } 
                else {
                    tpxl_video_frame_queue_close(&player->upload_queue);
                    player->decode_status = TPXL_THREAD_STATUS_FAILED;
                    break;
                }
            } 
            else if (result != TPXL_OK) {
                tpxl_video_frame_queue_close(&player->upload_queue);
                player->decode_status = TPXL_THREAD_STATUS_FAILED;
                break;
            }
        }

        TpxlVideoFrame frame = {0};
        result = tpxl_decode_video_packet(player->video, packet, &frame);

        av_packet_free(&packet);

        if (result == TPXL_EOF) {
            tpxl_video_frame_queue_close(&player->upload_queue);
            player->decode_status = TPXL_THREAD_STATUS_FINISHED;
            break;
        }

        if (result == TPXL_NEED_PACKET) {
            continue;
        }

        if (result != TPXL_OK) {
            tpxl_video_frame_queue_close(&player->upload_queue);
            player->decode_status = TPXL_THREAD_STATUS_FAILED;
            break;
        }

        result = tpxl_video_frame_queue_push(&player->upload_queue, &frame, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            tpxl_video_frame_free(&frame);
            player->decode_status = TPXL_THREAD_STATUS_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_video_frame_free(&frame);
            tpxl_video_frame_queue_close(&player->upload_queue);
            player->decode_status = TPXL_THREAD_STATUS_FAILED;
            break;
        }
    }

    return NULL;
}

static void* tpxl_upload_worker(void* arg) {

    TpxlVideoPlayer* player = (TpxlVideoPlayer*)arg;

    player->upload_status = TPXL_THREAD_STATUS_RUNNING;

    while (!atomic_load(&player->shutdown)) {

        TpxlResult result = TPXL_OK;

        TpxlVideoFrame video_frame = {0};
        result = tpxl_video_frame_queue_pop(&player->upload_queue, &video_frame, &player->shutdown);

        if (result == TPXL_QUEUE_CLOSED) {
            tpxl_video_frame_queue_close(&player->display_queue);
            player->upload_status = TPXL_THREAD_STATUS_FINISHED;
            break;
        }

        if (result == TPXL_SHUTDOWN) {
            player->upload_status = TPXL_THREAD_STATUS_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_video_frame_free(&video_frame);
            tpxl_video_frame_queue_close(&player->display_queue);
            player->upload_status = TPXL_THREAD_STATUS_FAILED;
            break;
        }

        uint32_t frame_id = atomic_fetch_add(&player->frame_id, 1);

        result = tpxl_renderer_upload(player->renderer, &video_frame.frame, frame_id);

        video_frame.id = frame_id;

        if (result != TPXL_OK) {
            tpxl_video_frame_free(&video_frame);
            tpxl_video_frame_queue_close(&player->display_queue);
            player->upload_status = TPXL_THREAD_STATUS_FAILED;
            break;
        }    

        result = tpxl_video_frame_queue_push(&player->display_queue, &video_frame, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            tpxl_video_frame_free(&video_frame);
            player->upload_status = TPXL_THREAD_STATUS_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_video_frame_free(&video_frame);
            tpxl_video_frame_queue_close(&player->display_queue);
            player->upload_status = TPXL_THREAD_STATUS_FAILED;
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

    tpxl_packet_queue_close(&player->video_packet_queue);
    tpxl_packet_queue_close(&player->audio_packet_queue);

    tpxl_video_frame_queue_close(&player->upload_queue);
    tpxl_video_frame_queue_close(&player->display_queue);

    tpxl_audio_player_destroy(&player->audio_player);

    if (player->demux_thread_created) {
        tpxl_thread_join(&player->demux_thread, NULL);
    }

    if (player->decode_thread_created) {
        tpxl_thread_join(&player->decode_thread, NULL);
    }

    if (player->upload_thread_created) {
        tpxl_thread_join(&player->upload_thread, NULL);
    }
    
    tpxl_destroy_packet_queue(&player->video_packet_queue);
    tpxl_destroy_packet_queue(&player->audio_packet_queue);

    tpxl_destroy_video_frame_queue(&player->upload_queue);
    tpxl_destroy_video_frame_queue(&player->display_queue);

    free(player);
}

TpxlResult tpxl_video_player_create(TpxlVideoPlayer** player, TpxlRenderer* renderer, TpxlVideo* video) {

    if (!video || !renderer || !player) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = calloc(1, sizeof(TpxlVideoPlayer));

    if (!*player) {
        return TPXL_OUT_OF_MEMORY;
    }

    (*player)->renderer = renderer;
    (*player)->video = video;
    (*player)->video_packet_queue = (TpxlPacketQueue){0};
    (*player)->audio_packet_queue = (TpxlPacketQueue){0};
    (*player)->upload_queue = (TpxlVideoFrameQueue){0};
    (*player)->display_queue = (TpxlVideoFrameQueue){0};
    (*player)->current_frame = (TpxlVideoFrame){0};
    (*player)->has_current_frame = false;
    (*player)->has_last_frame = false;
    
    (*player)->frame_count = tpxl_video_get_frame_count(video);;

    atomic_init(&(*player)->frame_id, 1);
    atomic_init(&(*player)->active, false);
    atomic_init(&(*player)->playing, false);
    atomic_init(&(*player)->frame_ready, false);
    atomic_init(&(*player)->shutdown, false);
    atomic_init(&(*player)->demux_status, TPXL_THREAD_STATUS_UNKNOWN);
    atomic_init(&(*player)->frames_played, 0);

    if (tpxl_init_packet_queue(&(*player)->video_packet_queue) != TPXL_OK ||
        tpxl_init_packet_queue(&(*player)->audio_packet_queue) != TPXL_OK) {

        tpxl_destroy_packet_queue(&(*player)->video_packet_queue);
        tpxl_destroy_packet_queue(&(*player)->audio_packet_queue);

        free(*player);
        *player = NULL;
        return TPXL_PLAYER_CREATION_FAILED;
    }

    if (tpxl_init_video_frame_queue(&(*player)->upload_queue) != TPXL_OK) {
        tpxl_destroy_packet_queue(&(*player)->video_packet_queue);
        tpxl_destroy_packet_queue(&(*player)->audio_packet_queue);

        free(*player);
        *player = NULL;
        return TPXL_PLAYER_CREATION_FAILED;
    }

    if (tpxl_init_video_frame_queue(&(*player)->display_queue) != TPXL_OK) {
        tpxl_destroy_packet_queue(&(*player)->video_packet_queue);
        tpxl_destroy_packet_queue(&(*player)->audio_packet_queue);

        tpxl_destroy_video_frame_queue(&(*player)->upload_queue);

        free(*player);
        *player = NULL;
        return TPXL_PLAYER_CREATION_FAILED;
    }

    if (tpxl_thread_create(&(*player)->demux_thread, tpxl_demux_worker, *player) != TPXL_OK) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_ERROR;
    }

    (*player)->demux_thread_created = true;

    if (tpxl_thread_create(&(*player)->decode_thread, tpxl_video_decode_worker, *player) != TPXL_OK) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_ERROR;
    }

    (*player)->decode_thread_created = true;

    if (tpxl_thread_create(&(*player)->upload_thread, tpxl_upload_worker, *player) != TPXL_OK) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_THREAD_ERROR;
    }

    (*player)->upload_thread_created = true;

    if (tpxl_video_audio_player_init(*player, video->audio) != TPXL_OK) {
        tpxl_abort_video_player_creation(*player);
        *player = NULL;
        return TPXL_PLAYER_CREATION_FAILED;
    }

    atomic_store(&(*player)->active, true);

    return TPXL_OK;
}

TpxlResult tpxl_video_player_update(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = TPXL_OK;

    if (!player->has_current_frame) {

        TpxlVideoFrame video_frame = {0};
        result = tpxl_video_frame_queue_pop(&player->display_queue, &video_frame, &player->shutdown);

        if (result == TPXL_QUEUE_CLOSED && player->upload_status == TPXL_THREAD_STATUS_FAILED) {
            atomic_store(&player->playing, false);
            atomic_store(&player->active, false);
            return TPXL_PLAYING_FAILED;
        }

        if (result == TPXL_QUEUE_CLOSED) {

            if (player->has_last_frame) {

                tpxl_renderer_delete_data(player->renderer, player->last_frame.id);

                tpxl_video_frame_free(&player->last_frame);
                player->has_last_frame = false;
            }

            atomic_store(&player->playing, false);
            atomic_store(&player->active, false);

            return TPXL_EOF;
        }

        if (result != TPXL_OK) {
            atomic_store(&player->playing, false);
            atomic_store(&player->active, false);
            return result;
        }

        player->current_frame = video_frame;
        player->has_current_frame = true;
    }

    atomic_store(&player->frame_ready, true);    

    double audio_clock = tpxl_audio_player_get_time(player->audio_player);
    double video_time = av_q2d(player->video->time_base) * player->current_frame.pts;
    double diff = video_time - audio_clock;

    if (diff > VIDEO_SYNC_MAX) {

        // Video is ahead
        // Wait for audio clock to catch up.
        double sleep_time = diff - VIDEO_SYNC_MAX;

        if (sleep_time > 0.0) {
            tpxl_sleep_us((uint64_t)(sleep_time * 1000000.0));
        }

        return TPXL_OK;
    }
    else if (diff < VIDEO_SYNC_MIN) {

        // Video is behind. 
        // Drop this frame.
        tpxl_renderer_delete_data(player->renderer, player->current_frame.id);
        tpxl_video_frame_free(&player->current_frame);
        player->has_current_frame = false;
    }
    else {
        result = tpxl_renderer_display(player->renderer, player->current_frame.id, player->row, player->column);

        if (result != TPXL_OK) {
            tpxl_renderer_delete_data(player->renderer, player->last_frame.id);
            tpxl_video_frame_free(&player->current_frame);
            player->has_current_frame = false;
            atomic_store(&player->playing, false);
            atomic_store(&player->active, false);
            return result;
        }

        atomic_fetch_add(&player->frames_played, 1);

        if (player->has_last_frame) {
           tpxl_renderer_delete_data(player->renderer, player->last_frame.id);
           tpxl_video_frame_free(&player->last_frame);
        }

        player->last_frame = player->current_frame;
        player->has_last_frame = true;

        player->has_current_frame = false;
    }

    return TPXL_OK;
}

static void* tpxl_video_play_worker(void* arg) {
    TpxlVideoPlayer* player = arg;

    TpxlResult result = TPXL_OK;

    player->play_status = TPXL_THREAD_STATUS_RUNNING;

    bool audio_started = false;

    while (atomic_load(&player->active)) {

        if (!atomic_load(&player->playing)) {
            tpxl_sleep_ms(100);
            continue;
        }

        if (atomic_load(&player->frame_ready) && !audio_started) {

            result = tpxl_audio_play(player->audio_player);
        
            if (result != TPXL_OK) {
                player->play_status = TPXL_THREAD_STATUS_FAILED;
                return NULL;
            }

            audio_started = true;
        }

        result = tpxl_video_player_update(player);

        if (result == TPXL_EOF) {
            player->play_status = TPXL_THREAD_STATUS_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            player->play_status = TPXL_THREAD_STATUS_FAILED;
            return NULL;
        }
    }

    return NULL;
}

TpxlResult tpxl_video_player_start(TpxlVideoPlayer* player, uint32_t row, uint32_t column) {
    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (player->started) {
        return TPXL_OK;
    }

    player->row = row;
    player->column = column;

    atomic_store(&player->playing, true);

    if (tpxl_thread_create(&player->play_thread, tpxl_video_play_worker, player) != TPXL_OK) {
        atomic_store(&player->playing, false);
        return TPXL_THREAD_ERROR;
    }   

    player->started = true;
    player->play_thread_created = true;

    return TPXL_OK;
}

TpxlResult tpxl_video_play(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = tpxl_audio_play(player->audio_player);

    if (result != TPXL_OK) {
        return result;
    }

    atomic_store(&player->playing, true);

    return TPXL_OK;
}

TpxlResult tpxl_video_pause(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }
 
    TpxlResult result = tpxl_audio_pause(player->audio_player);

    if (result != TPXL_OK) {
        return result;
    }

    atomic_store(&player->playing, false);

    return TPXL_OK;
}

TpxlResult tpxl_video_mute(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_audio_mute(player->audio_player);
}

TpxlResult tpxl_video_unmute(TpxlVideoPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_audio_unmute(player->audio_player);
}

bool tpxl_video_player_active(TpxlVideoPlayer* player) {

    if (!player) {
        return false;
    }

    return atomic_load(&player->active);
}

bool tpxl_video_player_playing(TpxlVideoPlayer* player) {

    if (!player) {
        return false;
    }

    return atomic_load(&player->playing);
}

bool tpxl_video_player_muted(TpxlVideoPlayer* player) {

    assert(player);
    return tpxl_audio_player_muted(player->audio_player);
}

double tpxl_video_player_get_time(TpxlVideoPlayer* player) {

    assert(player);
    return tpxl_audio_player_get_time(player->audio_player);
}

uint32_t tpxl_video_player_get_frames_played(TpxlVideoPlayer* player) {

    assert(player);
    return atomic_load(&player->frames_played);
}

void tpxl_video_player_destroy(TpxlVideoPlayer** player) {

    if (!*player) {
        return;
    }

    atomic_store(&(*player)->playing, false);
    atomic_store(&(*player)->active, false);
    atomic_store(&(*player)->shutdown, true);

    tpxl_packet_queue_close(&(*player)->video_packet_queue);
    tpxl_packet_queue_close(&(*player)->audio_packet_queue);
    tpxl_video_frame_queue_close(&(*player)->upload_queue);
    tpxl_video_frame_queue_close(&(*player)->display_queue);

    tpxl_audio_player_destroy(&(*player)->audio_player);

    tpxl_thread_join(&(*player)->demux_thread, NULL);
    tpxl_thread_join(&(*player)->decode_thread, NULL);
    tpxl_thread_join(&(*player)->upload_thread, NULL);
    tpxl_thread_join(&(*player)->play_thread, NULL);

    tpxl_destroy_packet_queue(&(*player)->video_packet_queue);
    tpxl_destroy_packet_queue(&(*player)->audio_packet_queue);
    tpxl_video_frame_queue_close(&(*player)->upload_queue);
    tpxl_video_frame_queue_close(&(*player)->display_queue);

    free(*player);
    *player = NULL;
}
