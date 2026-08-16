#ifndef TPXL_QUEUE_H
#define TPXL_QUEUE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "tpxl/type.h"

#define MAX_SLOT_COUNT 8

typedef struct {
    TpxlImage slots[MAX_SLOT_COUNT];
    size_t count;

    uint32_t write_idx;
    uint32_t read_idx;

} TpxlFrameQueue;

typedef struct {
    uint32_t slots[MAX_SLOT_COUNT];
    size_t count;

    uint32_t write_idx;
    uint32_t read_idx;

} TpxlFrameIDQueue;

bool tpxl_frame_queue_full(TpxlFrameQueue* queue);
TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, TpxlImage* frame);
TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, TpxlImage* out_frame);

bool tpxl_frame_id_queue_full(TpxlFrameIDQueue* queue);
TpxlResult tpxl_frame_id_queue_push(TpxlFrameIDQueue* queue, uint32_t frame_id);
TpxlResult tpxl_frame_id_queue_pop(TpxlFrameIDQueue* queue, uint32_t* out_frame_id);

#endif
