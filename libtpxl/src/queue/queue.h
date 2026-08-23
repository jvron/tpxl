#ifndef TPXL_QUEUE_H
#define TPXL_QUEUE_H

#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <bits/pthreadtypes.h>

#include <libavcodec/packet.h>

#include "tpxl/audio.h"
#include "tpxl/type.h"

#define MAX_SLOT_COUNT 32
#define MAX_PACKET_SLOT_COUNT 64

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

typedef struct {
    TpxlAudioFrame slots[MAX_SLOT_COUNT];

    size_t count;
    uint32_t write_idx;
    uint32_t read_idx;
    bool closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

} TpxlAudioFrameQueue;

typedef struct {
    AVPacket* packets[MAX_PACKET_SLOT_COUNT];

    size_t count;
    bool closed;

    size_t read_idx;
    size_t write_idx;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    
} TpxlPacketQueue;


TpxlResult tpxl_init_frame_queue(TpxlFrameQueue* queue);
void tpxl_frame_queue_close(TpxlFrameQueue* queue);
bool tpxl_frame_queue_full(TpxlFrameQueue* queue);
TpxlResult tpxl_frame_queue_push(TpxlFrameQueue* queue, TpxlImage* frame, atomic_bool* shutdown);
TpxlResult tpxl_frame_queue_pop(TpxlFrameQueue* queue, TpxlImage* out_frame, atomic_bool* shutdown);
void tpxl_destroy_frame_queue(TpxlFrameQueue* queue);

TpxlResult tpxl_init_frame_id_queue(TpxlFrameIDQueue* queue);
void tpxl_frame_id_queue_close(TpxlFrameIDQueue* queue);
bool tpxl_frame_id_queue_full(TpxlFrameIDQueue* queue);
TpxlResult tpxl_frame_id_queue_push(TpxlFrameIDQueue* queue, uint32_t frame_id, atomic_bool* shutdown);
TpxlResult tpxl_frame_id_queue_pop(TpxlFrameIDQueue* queue, uint32_t* out_frame_id, atomic_bool* shutdown);
void tpxl_destroy_frame_id_queue(TpxlFrameIDQueue* queue);

TpxlResult tpxl_init_audio_frame_queue(TpxlAudioFrameQueue* queue);
void tpxl_audio_frame_queue_close(TpxlAudioFrameQueue* queue);
bool tpxl_audio_frame_queue_full(TpxlAudioFrameQueue* queue);
TpxlResult tpxl_audio_frame_queue_push(TpxlAudioFrameQueue* queue, TpxlAudioFrame* frame, atomic_bool* shutdown);
TpxlResult tpxl_audio_frame_queue_pop(TpxlAudioFrameQueue* queue, TpxlAudioFrame* out_frame, atomic_bool* shutdown);
void tpxl_destroy_audio_frame_queue(TpxlAudioFrameQueue* queue);

TpxlResult tpxl_init_packet_queue(TpxlPacketQueue* queue);
void tpxl_packet_queue_close(TpxlPacketQueue* queue);
TpxlResult tpxl_packet_queue_push(TpxlPacketQueue* queue, AVPacket* packet, atomic_bool* shutdown);
TpxlResult tpxl_packet_queue_pop(TpxlPacketQueue* queue, AVPacket** out_packet, atomic_bool* shutdown);
void tpxl_destroy_packet_queue(TpxlPacketQueue* queue);

#endif
