#ifndef TPXL_INTERNAL_THREAD_H
#define TPXL_INTERNAL_THREAD_H

typedef enum {
    THREAD_RUNNING = 0,
    THREAD_WAITING,
    THREAD_ERROR,
    THREAD_FINISHED,
    
} TpxlThreadStatus;

#endif
