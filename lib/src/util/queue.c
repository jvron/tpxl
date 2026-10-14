
#include "tpxl/type.h"
#include "util/thread.h"

#include "queue.h"

TpxlResult tpxl_init_audio_frame_queue(TpxlAudioFrameQueue* queue) {

    if (!queue) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = tpxl_mutex_init(&queue->mutex);

    if (result != TPXL_OK) {
        return result;
    }

    result = tpxl_cond_init(&queue->not_empty);
    
    if (result != TPXL_OK) {
        tpxl_mutex_destroy(&queue->mutex);
        return result;
    }

    result = tpxl_cond_init(&queue->not_full);

    if (result != TPXL_OK) {
        tpxl_mutex_destroy(&queue->mutex);
        tpxl_cond_destroy(&queue->not_empty);
        return result;
    }

    queue->closed = false;

    return TPXL_OK;
}

void tpxl_audio_frame_queue_close(TpxlAudioFrameQueue* queue) {

    if (!queue) {
        return;
    }

    tpxl_mutex_lock(&queue->mutex);

    queue->closed = true;
    tpxl_cond_broadcast(&queue->not_empty);
    tpxl_cond_broadcast(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);
}

TpxlResult tpxl_audio_frame_queue_push(TpxlAudioFrameQueue* queue, TpxlAudioFrame* frame, atomic_bool* shutdown) {

    if (!queue || !frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    tpxl_mutex_lock(&queue->mutex);

    while (queue->count >= MAX_AUDIO_FRAME_COUNT && !queue->closed && !atomic_load(shutdown)) {
        tpxl_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    queue->slots[queue->write_idx] = *frame;
    queue->write_idx = (queue->write_idx + 1) % MAX_AUDIO_FRAME_COUNT;
    queue->count++;

    tpxl_cond_signal(&queue->not_empty);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

TpxlResult tpxl_audio_frame_queue_pop(TpxlAudioFrameQueue* queue, TpxlAudioFrame* out_frame, atomic_bool* shutdown) {

    if (!queue || !out_frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    tpxl_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->closed && !atomic_load(shutdown)) {
        tpxl_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    if (queue->count == 0 && queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }

    *out_frame = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = (TpxlAudioFrame){0};
    queue->read_idx = (queue->read_idx + 1) % MAX_AUDIO_FRAME_COUNT;
    queue->count--;

    tpxl_cond_signal(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

TpxlResult tpxl_audio_frame_queue_try_pop(TpxlAudioFrameQueue* queue, TpxlAudioFrame* out_frame, atomic_bool* shutdown) {

    if (!queue || !out_frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    tpxl_mutex_lock(&queue->mutex);

    if (queue->count == 0 && !queue->closed && !atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_EMPTY;
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    if (queue->count == 0 && queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }

    *out_frame = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = (TpxlAudioFrame){0};
    queue->read_idx = (queue->read_idx + 1) % MAX_AUDIO_FRAME_COUNT;
    queue->count--;

    tpxl_cond_signal(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

void tpxl_destroy_audio_frame_queue(TpxlAudioFrameQueue* queue) {

    if (!queue) {
        return;
    }

    tpxl_mutex_destroy(&queue->mutex);
    tpxl_cond_destroy(&queue->not_empty);
    tpxl_cond_destroy(&queue->not_full);
}

TpxlResult tpxl_init_video_frame_queue(TpxlVideoFrameQueue* queue) {

    if (!queue) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = tpxl_mutex_init(&queue->mutex);

    if (result != TPXL_OK) {
        return result;
    }

    result = tpxl_cond_init(&queue->not_empty);
    
    if (result != TPXL_OK) {
        tpxl_mutex_destroy(&queue->mutex);
        return result;
    }

    result = tpxl_cond_init(&queue->not_full);

    if (result != TPXL_OK) {
        tpxl_mutex_destroy(&queue->mutex);
        tpxl_cond_destroy(&queue->not_empty);
        return result;
    }

    queue->closed = false;

    return TPXL_OK;
}

void tpxl_video_frame_queue_close(TpxlVideoFrameQueue* queue) {

    if (!queue) {
        return;
    }

    tpxl_mutex_lock(&queue->mutex);

    queue->closed = true;
    tpxl_cond_broadcast(&queue->not_empty);
    tpxl_cond_broadcast(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);
}

TpxlResult tpxl_video_frame_queue_push(TpxlVideoFrameQueue* queue, TpxlVideoFrame* frame, atomic_bool* shutdown) {

    if (!queue || !frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    tpxl_mutex_lock(&queue->mutex);

    while (queue->count >= MAX_VIDEO_FRAME_COUNT && !queue->closed && !atomic_load(shutdown)) {
        tpxl_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    queue->slots[queue->write_idx] = *frame;
    queue->write_idx = (queue->write_idx + 1) % MAX_VIDEO_FRAME_COUNT;
    queue->count++;

    tpxl_cond_signal(&queue->not_empty);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

TpxlResult tpxl_video_frame_queue_pop(TpxlVideoFrameQueue* queue, TpxlVideoFrame* out_frame, atomic_bool* shutdown) {

    if (!queue || !out_frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    tpxl_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->closed && !atomic_load(shutdown)) {
        tpxl_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    if (queue->count == 0 && queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }

    *out_frame = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = (TpxlVideoFrame){0};
    queue->read_idx = (queue->read_idx + 1) % MAX_VIDEO_FRAME_COUNT;
    queue->count--;

    tpxl_cond_signal(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

void tpxl_destroy_video_frame_queue(TpxlVideoFrameQueue* queue) {

    if (!queue) {
        return;
    }

    tpxl_mutex_destroy(&queue->mutex);
    tpxl_cond_destroy(&queue->not_empty);
    tpxl_cond_destroy(&queue->not_full);
}

TpxlResult tpxl_init_packet_queue(TpxlPacketQueue* queue) {

    if (!queue) {
        return TPXL_INVALID_ARGUMENT;
    }
    TpxlResult result = tpxl_mutex_init(&queue->mutex);

    if (result != TPXL_OK) {
        return result;
    }

    result = tpxl_cond_init(&queue->not_empty);
    
    if (result != TPXL_OK) {
        tpxl_mutex_destroy(&queue->mutex);
        return result;
    }

    result = tpxl_cond_init(&queue->not_full);

    if (result != TPXL_OK) {
        tpxl_mutex_destroy(&queue->mutex);
        tpxl_cond_destroy(&queue->not_empty);
        return result;
    }

    queue->closed = false;

    return TPXL_OK;
}

void tpxl_packet_queue_close(TpxlPacketQueue* queue) {

    if (!queue) {
        return;
    }

    tpxl_mutex_lock(&queue->mutex);

    queue->closed = true;

    tpxl_cond_broadcast(&queue->not_empty);
    tpxl_cond_broadcast(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);
}

TpxlResult tpxl_packet_queue_push(TpxlPacketQueue* queue, AVPacket* packet, atomic_bool* shutdown) {

    if (!queue || !packet || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }
    
    tpxl_mutex_lock(&queue->mutex);

    while (queue->count >= MAX_PACKET_COUNT && !queue->closed && !atomic_load(shutdown)) {
        tpxl_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    queue->packets[queue->write_idx] = packet;
    queue->write_idx = (queue->write_idx + 1) % MAX_PACKET_COUNT;
    queue->count++;

    tpxl_cond_signal(&queue->not_empty);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

TpxlResult tpxl_packet_queue_pop(TpxlPacketQueue* queue, AVPacket** out_packet, atomic_bool* shutdown) {

    if (!queue || !out_packet || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    tpxl_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !queue->closed && !atomic_load(shutdown)) {
        tpxl_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    if (queue->count == 0 && queue->closed) {
        tpxl_mutex_unlock(&queue->mutex);
        return TPXL_QUEUE_CLOSED;
    }
    
    *out_packet = queue->packets[queue->read_idx];

    queue->packets[queue->read_idx] = NULL;
    queue->read_idx = (queue->read_idx + 1) % MAX_PACKET_COUNT;
    queue->count--;

    tpxl_cond_signal(&queue->not_full);

    tpxl_mutex_unlock(&queue->mutex);

    return TPXL_OK;
}

void tpxl_destroy_packet_queue(TpxlPacketQueue* queue) {

    if (!queue) {
        return;
    }

    tpxl_mutex_destroy(&queue->mutex);
    tpxl_cond_destroy(&queue->not_empty);
    tpxl_cond_destroy(&queue->not_full);
}
