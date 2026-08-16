#include "frame_queue.h"
#include "tpxl/type.h"
#include <stdbool.h>
#include <stdint.h>

bool tpxl_frame_queue_full(TpxlFrameQueue* queue) {

    if (!queue || queue->count < MAX_SLOT_COUNT) {
        return false;
    }

    return true;
}

TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, uint32_t frame_id) {

    if (!queue) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (queue->count >= MAX_SLOT_COUNT) {
        return TPXL_FRAME_QUEUE_FULL;
    }

    queue->slots[queue->write_idx] = frame_id;
    queue->write_idx = (queue->write_idx + 1) % MAX_SLOT_COUNT;
    queue->count++;

    return TPXL_OK;
}

TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, uint32_t* out_frame_id) {

    if (!queue || !out_frame_id) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (queue->count == 0) {
        return TPXL_FRAME_QUEUE_EMPTY;
    } 

    *out_frame_id = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = 0;

    queue->read_idx = (queue->read_idx + 1) % MAX_SLOT_COUNT;
    queue->count--;

    return TPXL_OK;
}

