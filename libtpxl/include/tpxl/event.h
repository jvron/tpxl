#ifndef TPXL_EVENT_H
#define TPXL_EVENT_H

#include <stdint.h>

#include "tpxl/type.h"

typedef enum {
    TPXL_KEY_UNKNOWN = 0,
    TPXL_KEY_W,
    TPXL_KEY_A,
    TPXL_KEY_S,
    TPXL_KEY_D,
    TPXL_KEY_Q,
    TPXL_KEY_H,
    TPXL_KEY_M,
    TPXL_KEY_P,
    TPXL_KEY_SPACE,
} TpxlKey;

typedef enum {
    TPXL_EVENT_NONE = 0,
    TPXL_EVENT_KEY,
    TPXL_EVENT_RESIZE,
} TpxlEventType;

typedef struct {
    TpxlEventType type;

    union {
        TpxlKey key;

        struct {
            uint32_t width;
            uint32_t height;

        } resize;
    };
} TpxlEvent;

TpxlResult tpxl_poll_event(TpxlEvent* event);

#endif
