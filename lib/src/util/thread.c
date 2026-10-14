#include <assert.h>

#include "tpxl/type.h"

#include "thread.h"

#ifdef _WIN32

#include <windows.h>
#include <stdlib.h>

typedef struct {
    TpxlThreadWorker worker;
    void* arg;
    void* result;
} TpxlThreadContext;

static DWORD WINAPI tpxl_thread_entry(LPVOID arg) {

    TpxlThreadContext* context = arg;

    context->result = context->worker(context->arg);

    return 0;
}

TpxlResult tpxl_thread_create(TpxlThread* thread, TpxlThreadWorker worker, void* arg) {

    assert(thread && worker);

    TpxlThreadContext* context = malloc(sizeof(*context));

    if (!context) {
        return TPXL_THREAD_ERROR;
    }

    context->worker = worker;
    context->arg = arg;
    context->result = NULL;

    thread->context = context;
    thread->handle = CreateThread(
        NULL,
        0,
        tpxl_thread_entry,
        context,
        0,
        NULL
    );

    if (!thread->handle) {
        free(context);
        thread->context = NULL;
        return TPXL_THREAD_ERROR;
    }

    return TPXL_OK;
}

void tpxl_thread_join(TpxlThread* thread, void** thread_return) {

    if (!thread || !thread->handle) {
        return;
    }

    DWORD wait_result = WaitForSingleObject(thread->handle, INFINITE);

    if (wait_result == WAIT_OBJECT_0) {
        if (thread_return) {
            *thread_return = thread->context->result;
        }

        free(thread->context);
        thread->context = NULL;

        CloseHandle(thread->handle);
        thread->handle = NULL;
    }
}

TpxlResult tpxl_mutex_init(TpxlMutex* mutex) {
    return InitializeCriticalSectionEx(mutex, 0, 0) ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_mutex_lock(TpxlMutex* mutex) {
    EnterCriticalSection(mutex);

    return TPXL_OK;
}

TpxlResult tpxl_mutex_unlock(TpxlMutex* mutex) {
    LeaveCriticalSection(mutex);

    return TPXL_OK;
}

void tpxl_mutex_destroy(TpxlMutex* mutex) {

    if (!mutex) {
        return;
    }

    DeleteCriticalSection(mutex);
}

TpxlResult tpxl_cond_init(TpxlCond* cond) {
    InitializeConditionVariable(cond);

    return TPXL_OK;
}

TpxlResult tpxl_cond_wait(TpxlCond* cond, TpxlMutex* mutex) {
    return SleepConditionVariableCS(cond, mutex, INFINITE) ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_cond_signal(TpxlCond* cond) {
    WakeConditionVariable(cond);

    return TPXL_OK;
}

TpxlResult tpxl_cond_broadcast(TpxlCond* cond) {
    WakeAllConditionVariable(cond);

    return TPXL_OK;
}

void tpxl_cond_destroy(TpxlCond* cond) {
    (void)cond;
}

#else

#include <pthread.h>

TpxlResult tpxl_thread_create(TpxlThread* thread, TpxlThreadWorker worker, void* arg) {
    return pthread_create(thread, NULL, worker, arg) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

void tpxl_thread_join(TpxlThread* thread, void** thread_return) {

    if (!thread) {
        return;
    }

    pthread_join(*thread, thread_return);
}

TpxlResult tpxl_mutex_init(TpxlMutex* mutex) {
    return pthread_mutex_init(mutex, NULL) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_mutex_lock(TpxlMutex* mutex) {
    return pthread_mutex_lock(mutex) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_mutex_unlock(TpxlMutex* mutex) {
    return pthread_mutex_unlock(mutex) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

void tpxl_mutex_destroy(TpxlMutex* mutex) {

    if (!mutex) {
        return;
    }

    pthread_mutex_destroy(mutex);
}

TpxlResult tpxl_cond_init(TpxlCond* cond) {
    return pthread_cond_init(cond, NULL) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_cond_signal(TpxlCond* cond) {
    return pthread_cond_signal(cond) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_cond_broadcast(TpxlCond* cond) {
    return pthread_cond_broadcast(cond) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

TpxlResult tpxl_cond_wait(TpxlCond* cond, TpxlMutex* mutex) {
    return pthread_cond_wait(cond, mutex) == 0 ? TPXL_OK : TPXL_THREAD_ERROR;
}

void tpxl_cond_destroy(TpxlCond* cond) {
    
    if (!cond) {
        return;
    }

    pthread_cond_destroy(cond);
}

#endif