#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void print_time(double seconds) {

    uint32_t minutes = (uint32_t)seconds / 60;
    uint32_t secs = (uint32_t)seconds % 60;

    printf("%02u:%02u", minutes, secs);
}

static void print_progress(double played, double duration) {

    const uint32_t bar_width = 28;

    double progress = 0.0;

    if (duration > 0.0) {
        progress = played / duration;
    }

    if (progress < 0.0) {
        progress = 0.0;
    }
    else if (progress > 1.0) {
        progress = 1.0;
    }

    uint32_t filled = (uint32_t)(progress * bar_width);

    putchar('[');

    for (uint32_t i = 0; i < bar_width; i++) {
        putchar(i < filled ? '#' : ' ');
    }

    putchar(']');
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
        tpxl_close_audio(audio);
        return EXIT_FAILURE;
    }

    result = tpxl_play_audio(player);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_audio_player(player);
        tpxl_close_audio(audio);
        return EXIT_FAILURE;
    }

    double duration = tpxl_get_audio_duration(audio);

    printf("\nPlaying: %s\n", get_filename(path));

    while (true) {

        TpxlEvent event;
        TpxlResult result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            printf("Error: %s\n", tpxl_result_to_string(result));
            tpxl_close_audio_player(player);
            tpxl_close_audio(audio);
            return EXIT_FAILURE;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) {
                break;
            }
        }

        double played = tpxl_get_audio_clock(player);

        printf("\r");
        print_time(played);
        printf(" ");

        print_progress(played, duration);

        printf(" ");
        print_time(duration);
        printf("    [q] Quit");

        fflush(stdout);

        if (played >= duration) {
            break;
        }

        tpxl_sleep_ms(50);
    }

    printf("\n");

    tpxl_close_audio_player(player);
    tpxl_close_audio(audio);

    return EXIT_SUCCESS;
}
