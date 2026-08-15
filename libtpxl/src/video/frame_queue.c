#include "frame_queue.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, TpxlImage* frame) {

    if (!queue || !frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (queue->count >= MAX_SLOT_COUNT) {
        return TPXL_FRAME_QUEUE_FULL;
    }

    queue->slots[queue->write_idx] = *frame;
    queue->write_idx = (queue->write_idx + 1) % MAX_SLOT_COUNT;
    queue->count++;

    return TPXL_OK;
}

TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, TpxlImage* out_frame) {

    if (!queue || !out_frame) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (queue->count == 0) {
        return TPXL_FRAME_QUEUE_EMPTY;
    } 

    *out_frame = queue->slots[queue->read_idx];

    queue->slots[queue->read_idx] = (TpxlImage){0};

    queue->read_idx = (queue->read_idx + 1) % MAX_SLOT_COUNT;
    queue->count--;

    return TPXL_OK;
}

