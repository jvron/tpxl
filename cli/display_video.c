#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "tpxl/renderer.h"
#include "tpxl/video.h"

#include "cli.h"

int display_video(const char* path, TpxlContext* context) {

    TpxlResult result = TPXL_OK;

    TpxlVideo* video = NULL;
    result = tpxl_open_video(path, &video);

    if (result != TPXL_OK) {
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_terminal(context);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(video);
        return EXIT_FAILURE;
    }

    uint32_t width, height;
    result = tpxl_get_video_source_dimensions(video, &width, &height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(video);
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_viewport(context, width, height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(video);
        return EXIT_FAILURE;
    }
    
    result = tpxl_video_set_output_size(video, context->viewport.width, context->viewport.height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(video);
        return EXIT_FAILURE;
    }

    uint32_t video_rows = (context->viewport.height + context->terminal.cell_height - 1) / context->terminal.cell_height;

    TpxlRenderer* renderer = NULL;

    TpxlFormat format;
    tpxl_get_video_format(video, &format);

    uint32_t output_width, output_height;
    result = tpxl_get_video_output_dimensions(video, &output_width, &output_height);

    result = tpxl_create_renderer(&renderer, context, output_width, output_height, format, TPXL_MEDIA_ANIMATED);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(video);
        return EXIT_FAILURE;
    }

    TpxlVideoPlayer* player = NULL;
    result = tpxl_create_video_player(&player, renderer, video);
    
    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_video(video);
        tpxl_destroy_renderer(renderer);
        return EXIT_FAILURE;
    }

    while (tpxl_video_playing(player)) {

        result = tpxl_update_video_player(player, renderer);

        if (result != TPXL_OK) {
            printf("\033[%uB", video_rows);
            printf("Error: %s\n", tpxl_result_to_string(result));
            tpxl_destroy_renderer(renderer);
            tpxl_close_video_player(player);
            tpxl_close_video(video);
            return EXIT_FAILURE;
        }
    }

    printf("\033[%uB", video_rows + 1);
    fflush(stdout);

    uint32_t frame_count = 0;
    tpxl_get_video_frame_count(video, &frame_count);
    tpxl_close_video_player(player);
    tpxl_destroy_renderer(renderer);
    tpxl_close_video(video);

    return EXIT_SUCCESS;
}
