#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tpxl/audio.h"
#include "tpxl/event.h"
#include "tpxl/image.h"
#include "tpxl/renderer.h"
#include "tpxl/terminal.h"

#include "cli.h"
#include "tpxl/util.h"

static const char* get_filename(const char* path) {

    const char* filename = strrchr(path, '/');

    if (filename) {
        return filename + 1;
    }

    return path;
}

int play_audio(const char* path, TpxlTerminal* terminal, TpxlBackend backend) {

    TpxlResult result = TPXL_OK;
    
    TpxlAudio* audio = NULL;
    TpxlAudioPlayer* player = NULL;

    result = tpxl_open_audio(path, &audio);

    if (result != TPXL_OK) {
        goto error;
    }

    bool has_thumbnail = true;
    TpxlImage thumbnail = {0};
    result = tpxl_audio_get_attached_thumbnail(audio, &thumbnail);

    if (result == TPXL_NOT_FOUND) {
        has_thumbnail = false;
    } else if (result != TPXL_OK) {
        goto error;
    }

    if (has_thumbnail) {
        
        uint32_t area_width = terminal->pixel_width;
        uint32_t area_height = terminal->pixel_height * terminal->cell_width / terminal->cell_height;

        uint32_t output_width, output_height;
        result = tpxl_scale_fit(
            area_width,
            area_height,
            thumbnail.width, 
            thumbnail.height, 
            &output_width,
            &output_height
        );

        if (result != TPXL_OK) {
            tpxl_free_frame(&thumbnail);
            goto error;
        }

        result = tpxl_resize_image(&thumbnail, output_width, output_height);
        
        if (result != TPXL_OK) {
            tpxl_free_frame(&thumbnail);
            goto error;
        }

        TpxlRendererConfig config;
        config.terminal = terminal;
        config.backend = backend;
        config.media_type = TPXL_MEDIA_IMAGE;
        config.render_width = output_width;
        config.render_height = output_height;
        config.format = thumbnail.format;
        
        TpxlRenderer* renderer = NULL;
        result = tpxl_create_renderer(&renderer, &config);

        if (result != TPXL_OK) {
            tpxl_free_frame(&thumbnail);
            goto error;
        }

        result = tpxl_renderer_direct_render(renderer, &thumbnail, terminal->cursor_row, terminal->cursor_column);

        if (result != TPXL_OK) {
            tpxl_free_frame(&thumbnail);
            tpxl_destroy_renderer(&renderer);
            goto error;
        }

        tpxl_free_frame(&thumbnail);
        tpxl_destroy_renderer(&renderer);
    }

    result = tpxl_create_audio_player(&player, audio);

    if (result != TPXL_OK) {
        goto error;
    }

    result = tpxl_play_audio(player);

    if (result != TPXL_OK) {
        goto error;
    }

    double duration = tpxl_get_audio_duration(audio);

    printf("\n\n%s\n\n", get_filename(path));

    while (tpxl_audio_player_active(player)) {

        bool playing = tpxl_audio_player_playing(player);
        bool muted = tpxl_audio_player_muted(player);

        TpxlEvent event;
        TpxlResult result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            goto error;
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

error:
    printf("\nError: %s\n", tpxl_result_to_string(result));
    tpxl_close_audio_player(&player);
    tpxl_close_audio(&audio);
    return EXIT_FAILURE;
}
