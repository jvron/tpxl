#ifndef TPXL_QUEUE_H
#define TPXL_QUEUE_H

#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <bits/pthreadtypes.h>

#include "tpxl/type.h"

#define MAX_SLOT_COUNT 8

typedef struct {
    TpxlImage slots[MAX_SLOT_COUNT];

    size_t count;
    uint32_t write_idx;
    uint32_t read_idx;
    bool closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

} TpxlFrameQueue;

typedef struct {
    uint32_t slots[MAX_SLOT_COUNT];

    size_t count;
    uint32_t write_idx;
    uint32_t read_idx;
    bool closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

} TpxlFrameIDQueue;

TpxlResult tpxl_init_frame_queue(TpxlFrameQueue* queue);
TpxlResult tpxl_frame_queue_close(TpxlFrameQueue* queue);
bool tpxl_frame_queue_full(TpxlFrameQueue* queue);
TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, TpxlImage* frame, atomic_bool* shutdown);
TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, TpxlImage* out_frame, atomic_bool* shutdown);
void tpxl_destroy_frame_queue(TpxlFrameQueue* queue);

TpxlResult tpxl_init_frame_id_queue(TpxlFrameIDQueue* queue);
TpxlResult tpxl_frame_id_queue_close(TpxlFrameIDQueue* queue);
bool tpxl_frame_id_queue_full(TpxlFrameIDQueue* queue);
TpxlResult tpxl_frame_id_queue_push(TpxlFrameIDQueue* queue, uint32_t frame_id, atomic_bool* shutdown);
TpxlResult tpxl_frame_id_queue_pop(TpxlFrameIDQueue* queue, uint32_t* out_frame_id, atomic_bool* shutdown);
void tpxl_destroy_frame_id_queue(TpxlFrameIDQueue* queue);

#endif
