#include <stdlib.h>
#include <stdbool.h>

#include <miniaudio/miniaudio.h>

#include "tpxl/audio.h"
#include "tpxl/type.h"
#include "internal/audio_internal.h"

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2

struct TpxlAudioPlayerImp {
    TpxlAudio* audio;

    ma_device device;
    bool playing;
};

static void audio_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {

    (void)input;

    TpxlAudioPlayer* player = (TpxlAudioPlayer*)device->pUserData;
    (void)player;

    float* samples = output;

    static float phase = 0.0f;

    for (ma_uint32 i = 0; i < frame_count; i++) {

        float sample = 0.2f * sinf(phase);

        samples[i * 2 + 0] = sample; // left
        samples[i * 2 + 1] = sample; // right

        phase += 2.0f * M_PI * 440.0f / AUDIO_SAMPLE_RATE;

        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }
    }
}

TpxlResult tpxl_create_audio_player(TpxlAudioPlayer** player, TpxlAudio* audio) {

    if (!player || !audio) {
        return TPXL_INVALID_ARGUMENT;
    }

    *player = malloc(sizeof(TpxlAudioPlayer));

    if (!*player) {
        return TPXL_OUT_OF_MEMORY;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.playback.channels = AUDIO_CHANNELS;
    config.pUserData = *player;
    config.dataCallback = audio_callback;

    ma_result result = ma_device_init(NULL, &config, &(*player)->device);

    if (result != MA_SUCCESS) {
        free(*player);
        *player = NULL;
        return TPXL_AUDIO_PLAYER_CREATION_FAILED;
    }

    (*player)->playing = true;
    (*player)->audio = audio;

    return TPXL_OK;
}

TpxlResult tpxl_play_audio(TpxlAudioPlayer* player) {

    if (!player) {
        return TPXL_INVALID_ARGUMENT;
    }

    ma_result result = ma_device_start(&player->device);

    if (result != MA_SUCCESS) {
        return TPXL_AUDIO_PLAYING_FAILED;
    }

    return TPXL_OK;
}

void tpxl_close_audio_player(TpxlAudioPlayer* player) {

    if (!player) {
        return;
    }

    ma_device_uninit(&player->device);

    free(player);
}
