#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tpxl/type.h"
#include "tpxl/util.h"
#include "tpxl/video.h"
#include "tpxl/event.h"
#include "tpxl/renderer.h"
#include "tpxl/terminal.h"

#include "cli.h"

int play_video(const char* path, TpxlTerminal* terminal, TpxlBackend backend) {

    TpxlResult result = TPXL_OK;

    TpxlVideo* video = NULL;
    TpxlRenderer* renderer = NULL;
    TpxlVideoPlayer* player = NULL;

    result = tpxl_open_video(path, &video);

    if (result != TPXL_OK) {
        goto error;
    }

    uint32_t width, height;
    result = tpxl_get_video_source_dimensions(video, &width, &height);

    if (result != TPXL_OK) {
        goto error;
    }
    
    uint32_t output_width, output_height;
    result = tpxl_scale_fit(
        terminal->pixel_width, 
        terminal->pixel_height * terminal->cell_width / terminal->cell_height, 
        width, 
        height, 
        &output_width,
        &output_height 
    );

    if (result != TPXL_OK) {
        goto error;
    }

    result = tpxl_video_set_output_size(video, output_width, output_height);

    if (result != TPXL_OK) {
        goto error;
    }

    uint32_t video_rows = (output_height + terminal->cell_height - 1) / terminal->cell_height;
    uint32_t video_cols = (output_width + terminal->cell_width - 1) / terminal->cell_width;

    result = tpxl_terminal_ensure_rows(terminal, video_rows + 1);

    if (result != TPXL_OK) {
        goto error;
    }

    TpxlRendererConfig config;
    config.backend = backend;
    config.terminal = terminal;
    config.media_type = TPXL_MEDIA_VIDEO;
    config.render_width = output_width;
    config.render_height = output_height;
    config.format = tpxl_get_video_format(video);
    
    result = tpxl_create_renderer(&renderer, &config);

    if (result != TPXL_OK) {
        goto error;
    }

    result = tpxl_create_video_player(&player, renderer, video);
    
    if (result != TPXL_OK) {
        goto error;
    }

    tpxl_sleep_ms(500);

    result = tpxl_start_video(player, terminal->cursor_row, terminal->cursor_column);

    if (result != TPXL_OK) {
        fprintf(stdout, "\033[%uB", video_rows);
        goto error;
    }

    double fps = tpxl_get_video_frame_rate(video);
    double duration = tpxl_get_video_duration(video);

    bool display_status_bar = true;

    while (tpxl_video_player_active(player)) {

        bool playing = tpxl_video_player_playing(player);
        bool muted = tpxl_video_player_muted(player);

        TpxlEvent event;
        result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            fprintf(stdout, "\033[%uB", video_rows);
            goto error;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) break;

            if (event.key == TPXL_KEY_H) {
                if (display_status_bar) {
                    display_status_bar = false;
                    fprintf(stdout, "\033[%uB", video_rows + 1);
                    fprintf(stdout, "\033[2K\r");
                } else {
                    display_status_bar = true;
                }
            } else if (event.key == TPXL_KEY_P) {
                if (playing) {
                    tpxl_pause_video(player);
                } else {
                    tpxl_play_video(player);
                }
            } else if (event.key == TPXL_KEY_M) {
                if (muted) {
                    tpxl_unmute_video(player);
                } else {
                    tpxl_mute_video(player);
                }
            }
        }

        if (display_status_bar) {
            double current = tpxl_get_video_time(player);

            fprintf(stdout, "\033[%uB", video_rows + 1);
            fprintf(stdout, "\r\033[K");
            
            int current_sec = (int)current;
            int duration_sec = (int)duration;

            fprintf(
                stdout,
                "\r%02d:%02d/",
                current_sec / 60,
                current_sec % 60
            );

            fprintf(
                stdout,
                "%02d:%02d ",
                duration_sec / 60,
                duration_sec % 60
            );

            fprintf(
                stdout,
                "frame=%u fps=%.1f  [p] %s [m] %s [h] hide [q] quit", 
                tpxl_get_frames_played(player), 
                fps, 
                playing ? "pause" : "play",
                muted ? "unmute" : "mute"
            );
            
            fprintf(stdout, "\033[%uA\033[%uG", video_rows + 1, video_cols);
            fflush(stdout);
        }

        tpxl_sleep_ms(200);
    }

    fprintf(stdout, "\033[%uB", video_rows + 2);
    tpxl_close_video_player(&player);
    tpxl_destroy_renderer(&renderer);
    tpxl_close_video(&video);
    return EXIT_SUCCESS;

error:
    fprintf(stderr, "Error: %s\n", tpxl_result_to_string(result));
    tpxl_close_video_player(&player);
    tpxl_destroy_renderer(&renderer);
    tpxl_close_video(&video);
    return EXIT_FAILURE;
}