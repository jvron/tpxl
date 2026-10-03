#include <stdio.h>
#include <stdlib.h>

#include "tpxl/type.h"
#include "tpxl/util.h"
#include "tpxl/event.h"
#include "tpxl/animation.h"
#include "tpxl/renderer.h"
#include "tpxl/terminal.h"

#include "cli.h"

int display_gif(const char* path, TpxlTerminal* terminal, TpxlBackend backend, bool print_info) {

    TpxlResult result = TPXL_OK;

    TpxlRenderer* renderer = NULL;
    TpxlAnimationPlayer* player = NULL;

    TpxlAnimation animation; 

    result = tpxl_load_animation(path, &animation);
    if (result != TPXL_OK) goto error;

    if (print_info) {
        tpxl_print_animation_info(&animation);
        tpxl_free_animation(&animation);
        return EXIT_SUCCESS;
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
    if (result != TPXL_OK) goto error;

    result = tpxl_resize_animation(&animation, output_width, output_height);
    if (result != TPXL_OK) goto error;

    uint32_t animation_rows = (output_height + terminal->cell_height - 1) / terminal->cell_height;

    result = tpxl_terminal_ensure_rows(terminal, animation_rows + 1);
    if (result != TPXL_OK) goto error;

    TpxlRendererConfig config;
    config.media_type = TPXL_MEDIA_ANIMATION;
    config.backend = backend;
    config.terminal = terminal;
    config.render_width = output_width;
    config.render_height = output_height;
    config.format = animation.format;

    result = tpxl_create_renderer(&renderer, &config);
    if (result != TPXL_OK) goto error;
    
    result = tpxl_create_animation_player(&player, renderer, &animation);
    if (result != TPXL_OK) goto error;

    printf("\033[%uB", animation_rows);
    printf("\n[q] Quit");
    printf("\033[%uA", animation_rows);
    fflush(stdout);

    result = tpxl_play_animation(player, terminal->cursor_row, terminal->cursor_column);

    while(true) {

        TpxlEvent event;
        result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            printf("\033[%uB", animation_rows + 1);
            goto error;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) {
                break;
            }
        }

        tpxl_sleep_ms(100);
    }

    tpxl_close_animation_player(&player);

    result = tpxl_renderer_direct_render(
        renderer, 
        &animation.frames[0].frame, 
        terminal->cursor_row, 
        terminal->cursor_column
    );
    if (result != TPXL_OK) {
        printf("\033[%uB", animation_rows + 1);
        goto error;
    }

    printf("\033[%uB", animation_rows + 1);

    tpxl_destroy_renderer(&renderer);
    tpxl_free_animation(&animation);
    return EXIT_SUCCESS;

error:
    printf("Error: %s\n", tpxl_result_to_string(result));
    tpxl_close_animation_player(&player);
    tpxl_destroy_renderer(&renderer);
    tpxl_free_animation(&animation);
    return EXIT_FAILURE;
}
