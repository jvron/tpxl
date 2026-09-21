#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tpxl/type.h"
#include "tpxl/audio.h"
#include "tpxl/event.h"

#include "cli.h"
#include "tpxl/util.h"

static const char* get_filename(const char* path) {

    const char* filename = strrchr(path, '/');

    if (filename) {
        return filename + 1;
    }

    return path;
}

int play_audio(const char* path) {

    TpxlResult result = TPXL_OK;
    
    TpxlAudio* audio = NULL;
    result = tpxl_open_audio(path, &audio);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    TpxlAudioPlayer* player = NULL;
    result = tpxl_create_audio_player(&player, audio);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_audio(&audio);
        return EXIT_FAILURE;
    }

    result = tpxl_play_audio(player);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_audio_player(&player);
        tpxl_close_audio(&audio);
        return EXIT_FAILURE;
    }

    double duration = tpxl_get_audio_duration(audio);

    printf("\n%s\n\n", get_filename(path));

    while (tpxl_audio_player_active(player)) {

        bool playing = tpxl_audio_player_playing(player);
        bool muted = tpxl_audio_player_muted(player);

        TpxlEvent event;
        TpxlResult result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            printf("Error: %s\n", tpxl_result_to_string(result));
            tpxl_close_audio_player(&player);
            tpxl_close_audio(&audio);
            return EXIT_FAILURE;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) {
                break;
            }
            if (event.key == TPXL_KEY_P) {
                if (playing) {
                    tpxl_pause_audio(player);
                } else {
                    tpxl_play_audio(player);
                }
            } else if (event.key == TPXL_KEY_M) {
                if (muted) {
                    tpxl_unmute_audio(player);
                } else {
                    tpxl_mute_audio(player);
                }
            }
        }

        double played = tpxl_get_audio_clock(player);

        int played_sec = (int)played;
        int duration_sec = (int)duration;

        printf("\r\033[K");

        printf(
            "%s %02d:%02d/%02d:%02d  [p] %s [m] %s [q] quit",
            playing ? "Playing" : "Paused",
            played_sec / 60,
            played_sec % 60,
            duration_sec / 60,
            duration_sec % 60,
            playing ? "pause" : "play",
            muted ? "unmute" : "mute"
        );
        fflush(stdout);

        tpxl_sleep_ms(100);
    }

    printf("\n");

    tpxl_close_audio_player(&player);
    tpxl_close_audio(&audio);

    return EXIT_SUCCESS;
}
