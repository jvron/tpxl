#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <threads.h>
#include <unistd.h>

#include "tpxl/image.h"
#include "tpxl/renderer.h"
#include "tpxl/type.h"
#include "tpxl/video.h"
#include "queue.h"

typedef enum {
    THREAD_RUNNING = 0,
    THREAD_WAITING,
    THREAD_ERROR,
    THREAD_FINISHED,
    
} TpxlThreadStatus;

struct TpxlVideoPlayerImp {
    TpxlRenderer* renderer;

    TpxlVideo* video;
    uint32_t frame_count;

    uint32_t frame_id;
    bool playing;

    TpxlFrameIDQueue id_queue;
    TpxlFrameQueue frame_queue;

    atomic_bool shutdown;
    TpxlThreadStatus decode_status;
    TpxlThreadStatus upload_status;

    pthread_t decode_thread;
    pthread_t upload_thread;
};

void* tpxl_decode_worker(void* arg) {

    TpxlVideoPlayer* player = (TpxlVideoPlayer*)arg; 

    player->decode_status = THREAD_RUNNING;

    while (!atomic_load(&player->shutdown)) {

        TpxlImage frame = {0};
        TpxlResult result = tpxl_decode_video_frame(player->video, &frame);

        if (result == TPXL_EOF) {
            tpxl_frame_queue_close(&player->frame_queue);
            player->decode_status = THREAD_FINISHED;
            break;
        }
        if (result != TPXL_OK) {
            tpxl_frame_queue_close(&player->frame_queue);
            player->decode_status = THREAD_ERROR;
            break;
        }

        result = tpxl_frame_queue_push(&player->frame_queue, &frame, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
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

void* tpxl_upload_worker(void* arg1) {

    TpxlVideoPlayer* player = (TpxlVideoPlayer*)arg1;

    player->upload_status = THREAD_RUNNING;

    while (!atomic_load(&player->shutdown)) {

        TpxlResult result = TPXL_OK;

        TpxlImage frame = {0};
        result = tpxl_frame_queue_pop(&player->frame_queue, &frame, &player->shutdown);

        if (result == TPXL_QUEUE_CLOSED) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload_status = THREAD_FINISHED;
            break;
        }

        if (result == TPXL_SHUTDOWN) {
            player->upload_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_free_frame(&frame);
            player->upload_status = THREAD_ERROR;
            break;
        }

        result = tpxl_renderer_upload(player->renderer, &frame, player->frame_id);
        tpxl_free_frame(&frame);

        if (result != TPXL_OK) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload_status = THREAD_ERROR;
            break;
        }    

        result = tpxl_frame_id_queue_push(&player->id_queue, player->frame_id, &player->shutdown);

        if (result == TPXL_SHUTDOWN) {
            player->upload_status = THREAD_FINISHED;
            break;
        }

        if (result != TPXL_OK) {
            tpxl_frame_id_queue_close(&player->id_queue);
            player->upload_status = THREAD_ERROR;
            break;
        }
        
        player->frame_id++;
    }

    return NULL;
}

TpxlResult tpxl_create_video_player(TpxlVideoPlayer** player, TpxlRenderer* renderer, TpxlVideo* video) {

    if (!video || !renderer || !player) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = malloc(sizeof(TpxlVideoPlayer));

    if (!*player) {
        return TPXL_OUT_OF_MEMORY;
    }

    (*player)->renderer = renderer;
    (*player)->video = video;
    (*player)->id_queue = (TpxlFrameIDQueue){0};
    (*player)->frame_queue = (TpxlFrameQueue){0};
    (*player)->frame_id = 1;
    (*player)->playing = true;
    
    uint32_t count = 0;
    tpxl_get_video_frame_count(video, &count);
    (*player)->frame_count = count;

    atomic_store(&(*player)->shutdown, false);

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

    int result = 0;
    result = pthread_create(&(*player)->decode_thread, NULL, tpxl_decode_worker, *player);

    if (result != 0) {
        tpxl_destroy_frame_queue(&(*player)->frame_queue);
        tpxl_destroy_frame_id_queue(&(*player)->id_queue);
        free(*player);
        *player = NULL;
        return TPXL_THREAD_ERROR;
    }

    result = pthread_create(&(*player)->upload_thread, NULL, tpxl_upload_worker, *player);

    if (result != 0) {
        pthread_join((*player)->decode_thread, NULL);
        tpxl_destroy_frame_queue(&(*player)->frame_queue);
        tpxl_destroy_frame_id_queue(&(*player)->id_queue);
        free(*player);
        *player = NULL;
        return TPXL_THREAD_ERROR;
    }

    return TPXL_OK;
}

TpxlResult tpxl_update_video_player(TpxlVideoPlayer* player, TpxlRenderer* renderer) {

    if (!player || !renderer) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (player->frame_id == player->frame_count) {
        player->playing = false;
    }

    return TPXL_OK;
}

TpxlResult tpxl_video_player_pop_frame(TpxlVideoPlayer* player, uint32_t* out_frame_id) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    return tpxl_frame_id_queue_pop(&player->id_queue, out_frame_id, &player->shutdown);
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

    pthread_join(player->decode_thread, NULL);
    pthread_join(player->upload_thread, NULL);

    tpxl_destroy_frame_queue(&player->frame_queue);
    tpxl_destroy_frame_id_queue(&player->id_queue);

    free(player);
}
