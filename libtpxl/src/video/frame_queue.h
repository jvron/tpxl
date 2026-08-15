#ifndef TPXL_FRAME_QUEUE_H
#define TPXL_FRAME_QUEUE_H

#include <stdint.h>
#include <stdio.h>

#include "tpxl/type.h"

#define MAX_SLOT_COUNT 8

typedef struct {
    TpxlImage slots[MAX_SLOT_COUNT];
    size_t count;

    uint32_t write_idx;
    uint32_t read_idx;

} TpxlFrameQueue;

TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, TpxlImage* frame);
TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, TpxlImage* frame);


#endif
