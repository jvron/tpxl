#include <stdio.h>
#include <stdlib.h>

#include "tpxl/type.h"
#include "tpxl/util.h"
#include "tpxl/image.h"
#include "tpxl/event.h"
#include "tpxl/animation.h"
#include "tpxl/renderer.h"
#include "tpxl/terminal.h"

#include "cli.h"

int display_gif(const char* path, TpxlTerminal* terminal, TpxlBackend backend, bool print_info) {

    TpxlResult result = TPXL_OK;

    TpxlAnimation animation; 
    result = tpxl_load_gif(path, &animation);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    if (print_info) {
        tpxl_print_animation_info(&animation);
        tpxl_free_animation(&animation);
        return EXIT_SUCCESS;
    }
    
    TpxlAnimationPlayer player;
    result = tpxl_init_animation_player(&player, &animation);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    uint32_t area_width = terminal->pixel_width;
    uint32_t area_height = terminal->pixel_height * terminal->cell_width / terminal->cell_height;

    uint32_t output_width, output_height;
    result = tpxl_scale_fit(
        area_width, 
        area_height, 
        animation.width, 
        animation.height, 
        &output_width,
        &output_height 
    );

    TpxlRendererConfig config;
    config.backend = backend;
    config.media_type = TPXL_MEDIA_ANIMATION;
    config.terminal = terminal;
    config.render_width = output_width;
    config.render_height = output_height;
    config.format = animation.format;

    TpxlRenderer* renderer = NULL;
    result = tpxl_create_renderer(&renderer, &config);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    uint32_t animation_rows = (output_height + terminal->cell_height - 1) / terminal->cell_height;

    printf("\033[%uB", animation_rows);
    printf("\n[q] Quit");
    printf("\033[%uA", animation_rows);
    fflush(stdout);

    uint64_t previous = tpxl_get_time_ms();

    while(true) {

        TpxlEvent event;
        result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            printf("\033[%uB", animation_rows);
            printf("Error: %s\n", tpxl_result_to_string(result));
            tpxl_destroy_renderer(&renderer);
            tpxl_free_animation(&animation);
            return EXIT_FAILURE;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) {
                break;
            }
        }
        
        uint64_t now = tpxl_get_time_ms();
        uint64_t delta = now - previous;
        previous = now;

        bool frame_changed = tpxl_update_animation_player(&player, delta);

        if (frame_changed) {

            TpxlImage* frame = tpxl_get_animation_frame(&player);
            
            result = tpxl_resize_image(frame, output_width, output_height);

           if (result != TPXL_OK) {
                printf("\033[%uB", animation_rows);
                printf("Error: %s\n", tpxl_result_to_string(result));
                tpxl_destroy_renderer(&renderer);
                tpxl_free_animation(&animation);
                return EXIT_FAILURE;
            }
    
            result = tpxl_renderer_direct_render(renderer, frame, terminal->cursor_row, terminal->cursor_column);

            if (result != TPXL_OK) {
                printf("\033[%uB", animation_rows);
                printf("Error: %s\n", tpxl_result_to_string(result));
                tpxl_destroy_renderer(&renderer);
                tpxl_free_animation(&animation);
                return EXIT_FAILURE;
            }
        }

        uint32_t remaining = player.animation->delays[player.current_frame] - player.elapsed;

        tpxl_sleep_ms(remaining);
    }

    printf("\033[%uB", animation_rows + 1);
    fflush(stdout);

    tpxl_destroy_renderer(&renderer);
    tpxl_free_animation(&animation);

    return EXIT_SUCCESS;
}
