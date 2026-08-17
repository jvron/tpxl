#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#include "queue.h"
#include "tpxl/type.h"

TpxlResult tpxl_init_frame_queue(TpxlFrameQueue* queue) {

    if (!queue) {
        return TPXL_INVALID_ARGUMENT;
    }

    int result = 0;

    result = pthread_mutex_init(&queue->mutex, NULL);

    if (result != 0) {
        return TPXL_ERROR;
    }

    result = pthread_cond_init(&queue->not_empty, NULL);

    if (result != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return TPXL_ERROR;
    }

    result = pthread_cond_init(&queue->not_full, NULL);

    if (result != 0) {
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->not_empty);
        return TPXL_ERROR;
    }

    return TPXL_OK;
}

bool tpxl_frame_queue_full(TpxlFrameQueue* queue) {

    if (!queue) {
        return false;
    }
    pthread_mutex_lock(&queue->mutex);

    bool full = queue->count >= MAX_SLOT_COUNT;

    pthread_mutex_unlock(&queue->mutex);
    return full;
}

TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, TpxlImage* frame, atomic_bool* shutdown) {

    if (!queue || !frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }
    
    pthread_mutex_lock(&queue->mutex);

    while (queue->count >= MAX_SLOT_COUNT && !atomic_load(shutdown)) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        pthread_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    queue->slots[queue->write_idx] = *frame;
    queue->write_idx = (queue->write_idx + 1) % MAX_SLOT_COUNT;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);
    return TPXL_OK;
}

TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, TpxlImage* out_frame, atomic_bool* shutdown) {

    if (!queue || !out_frame || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !atomic_load(shutdown)) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        pthread_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }
    
    *out_frame = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = (TpxlImage){0};

    queue->read_idx = (queue->read_idx + 1) % MAX_SLOT_COUNT;
    queue->count--;

    pthread_cond_signal(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);
    return TPXL_OK;
}

void tpxl_destroy_frame_queue(TpxlFrameQueue* queue) {

    if (!queue) {
        return;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

TpxlResult tpxl_init_frame_id_queue(TpxlFrameIDQueue* queue) {

    if (!queue) {
        return TPXL_INVALID_ARGUMENT;
    }

    int result = 0;

    result = pthread_mutex_init(&queue->mutex, NULL);

    if (result != 0) {
        return TPXL_ERROR;
    }

    result = pthread_cond_init(&queue->not_empty, NULL);

    if (result != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return TPXL_ERROR;
    }

    result = pthread_cond_init(&queue->not_full, NULL);

    if (result != 0) {
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->not_empty);
        return TPXL_ERROR;
    }

    return TPXL_OK;
}

bool tpxl_frame_id_queue_full(TpxlFrameIDQueue* queue) {

    if (!queue) {
        return false;
    }
    pthread_mutex_lock(&queue->mutex);

    bool full = queue->count >= MAX_SLOT_COUNT;

    pthread_mutex_unlock(&queue->mutex);
    return full;
}

TpxlResult tpxl_frame_id_queue_push(TpxlFrameIDQueue* queue, uint32_t frame_id, atomic_bool* shutdown) {

    if (!queue || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count >= MAX_SLOT_COUNT && !atomic_load(shutdown)) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        pthread_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    queue->slots[queue->write_idx] = frame_id;
    queue->write_idx = (queue->write_idx + 1) % MAX_SLOT_COUNT;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);
    return TPXL_OK;
}

TpxlResult tpxl_frame_id_queue_pop(TpxlFrameIDQueue* queue, uint32_t* out_frame_id, atomic_bool* shutdown) {

    if (!queue || !out_frame_id || !shutdown) {
        return TPXL_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == 0 && !atomic_load(shutdown)) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (atomic_load(shutdown)) {
        pthread_mutex_unlock(&queue->mutex);
        return TPXL_SHUTDOWN;
    }

    *out_frame_id = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = 0;

    queue->read_idx = (queue->read_idx + 1) % MAX_SLOT_COUNT;
    queue->count--;

    pthread_cond_signal(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);
    return TPXL_OK;
}

void tpxl_destroy_frame_id_queue(TpxlFrameIDQueue* queue) {

    if (!queue) {
        return;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}
