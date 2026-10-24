#ifndef TPXL_INTERNAL_THREAD_H
#define TPXL_INTERNAL_THREAD_H

#include "tpxl/type.h"

typedef enum {
    TPXL_THREAD_STATUS_UNKNOWN = 0,
    TPXL_THREAD_STATUS_RUNNING,
    TPXL_THREAD_STATUS_WAITING,
    TPXL_THREAD_STATUS_FINISHED,
    TPXL_THREAD_STATUS_FAILED,
} TpxlThreadStatus;

#ifdef _WIN32

#include <windows.h>

typedef struct {
    HANDLE handle;
    void* result;
    void* context;
} TpxlThread;

typedef CRITICAL_SECTION TpxlMutex;
typedef CONDITION_VARIABLE TpxlCond;

#else
#include <bits/pthreadtypes.h>

typedef pthread_t TpxlThread;
typedef pthread_mutex_t TpxlMutex;
typedef pthread_cond_t TpxlCond;

#endif

typedef void* (*TpxlThreadWorker)(void* arg);

TpxlResult tpxl_thread_create(TpxlThread* thread, TpxlThreadWorker worker, void* arg);
void tpxl_thread_join(TpxlThread* thread, void** thread_return);

TpxlResult tpxl_mutex_init(TpxlMutex* mutex);
TpxlResult tpxl_mutex_lock(TpxlMutex* mutex);
TpxlResult tpxl_mutex_unlock(TpxlMutex* mutex);
void tpxl_mutex_destroy(TpxlMutex* mutex);

TpxlResult tpxl_cond_init(TpxlCond* cond);
TpxlResult tpxl_cond_signal(TpxlCond* cond);
TpxlResult tpxl_cond_broadcast(TpxlCond* cond);
TpxlResult tpxl_cond_wait(TpxlCond* cond, TpxlMutex* mutex);
void tpxl_cond_destroy(TpxlCond* cond);

#endif
