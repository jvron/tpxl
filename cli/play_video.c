#include <stdio.h>
#include <stdlib.h>

#include "tpxl/type.h"
#include "tpxl/util.h"
#include "tpxl/video.h"
#include "tpxl/event.h"
#include "tpxl/renderer.h"

#include "cli.h"

int play_video(const char* path, TpxlContext* context) {

    TpxlResult result = TPXL_OK;

    TpxlVideo* video = NULL;
    result = tpxl_open_video(path, &video);

    if (result != TPXL_OK) {
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_terminal(context);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        return EXIT_FAILURE;
    }

    uint32_t width, height;
    result = tpxl_get_video_source_dimensions(video, &width, &height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_viewport(context, width, height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        return EXIT_FAILURE;
    }
    
    result = tpxl_video_set_output_size(video, context->viewport.width, context->viewport.height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        return EXIT_FAILURE;
    }

    uint32_t video_rows = (context->viewport.height + context->terminal.cell_height - 1) / context->terminal.cell_height;
    uint32_t video_cols = context->viewport.x / context->terminal.cell_width;

    TpxlFormat format;
    tpxl_get_video_format(video, &format);

    uint32_t output_width, output_height;
    result = tpxl_get_video_output_dimensions(video, &output_width, &output_height);

    TpxlRenderer* renderer = NULL;
    result = tpxl_create_renderer(&renderer, context, output_width, output_height, format, TPXL_MEDIA_VIDEO);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        return EXIT_FAILURE;
    }

    TpxlImage background = {0};
    background.width = output_width;
    background.height = output_height;
    background.format = TPXL_FORMAT_RGB;
    background.pixels = calloc(1, output_width * output_height * tpxl_format_to_channels(TPXL_FORMAT_RGB));

    if (!background.pixels) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        tpxl_destroy_renderer(&renderer);
        return EXIT_FAILURE;
    }

    result = tpxl_renderer_render(renderer, &background);
    free(background.pixels);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        tpxl_destroy_renderer(&renderer);
        return EXIT_FAILURE;
    }

    TpxlVideoPlayer* player = NULL;
    result = tpxl_create_video_player(&player, renderer, video);
    
    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(&video);
        tpxl_destroy_renderer(&renderer);
        return EXIT_FAILURE;
    }

    result = tpxl_start_video(player);

    if (result != TPXL_OK) {
        printf("\033[%uB", video_rows);
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_destroy_renderer(&renderer);
        tpxl_close_video_player(&player);
        tpxl_close_video(&video);
        return EXIT_FAILURE;
    }

    double fps = tpxl_get_video_frame_rate(video);
    double duration = tpxl_get_video_duration(video);

    bool display_status_bar = true;

    while (tpxl_video_player_active(player)) {

        bool playing = tpxl_video_player_playing(player);
        bool muted = tpxl_video_player_muted(player);

        TpxlEvent event;
        TpxlResult result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            printf("\033[%uB", video_rows);
            printf("Error: %s\n", tpxl_result_to_string(result));
            tpxl_close_video_player(&player);
            tpxl_destroy_renderer(&renderer);
            tpxl_close_video(&video);
            return EXIT_FAILURE;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) break;

            if (event.key == TPXL_KEY_H) {
                if (display_status_bar) {
                    display_status_bar = false;
                    printf("\033[%uB", video_rows + 1);
                    printf("\033[2K\r");
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

            printf("\033[%uB", video_rows + 1);

            printf("\r\033[K");
            
            int current_sec = (int)current;
            int duration_sec = (int)duration;

            printf(
                "\r%02d:%02d/",
                current_sec / 60,
                current_sec % 60
            );

            printf(
                "%02d:%02d ",
                duration_sec / 60,
                duration_sec % 60
            );

            printf(
                "frame=%u fps=%.1f  [p] %s   [m] %s   [h] hide   [q] quit", 
                tpxl_get_frames_played(player), 
                fps, 
                playing ? "pause" : "play",
                muted ? "unmute" : "mute"
            );
            
            printf("\033[%uA\033[%uG", video_rows + 1, video_cols);
            fflush(stdout);
        }

        tpxl_sleep_ms(100);
    }

    printf("\033[%uB", video_rows + 2);

    tpxl_close_video_player(&player);
    tpxl_destroy_renderer(&renderer);
    tpxl_close_video(&video);

    return EXIT_SUCCESS;
}
