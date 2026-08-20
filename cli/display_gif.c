#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "tpxl/animation.h"
#include "tpxl/renderer.h"
#include "tpxl/event.h"

#include "cli.h"

static uint64_t get_time_ms(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

static void sleep_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}

int display_gif(const char* path, TpxlContext* context, bool print_info) {

    TpxlResult result;

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

    result = tpxl_update_context_terminal(context);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_viewport(context, animation.width, animation.height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }
    
    TpxlAnimator animator;
    result = tpxl_init_animator(&animator, &animation);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    TpxlRenderer* renderer = NULL;

    result = tpxl_create_renderer(&renderer, context,animation.width, animation.height, animation.format, TPXL_MEDIA_ANIMATED);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    uint32_t image_rows = (context->viewport.height + context->terminal.cell_height - 1) / context->terminal.cell_height;

    printf("\033[%uB", image_rows);
    printf("\n[q] Quit");
    printf("\033[%uA", image_rows);
    fflush(stdout);

    bool running = true;

    uint64_t previous = get_time_ms();

    while(running) {

        TpxlEvent event;
        TpxlResult result = tpxl_poll_event(&event);

        if (result != TPXL_OK) {
            printf("\033[%uB", image_rows);
            printf("Error: %s\n", tpxl_result_to_string(result));
            tpxl_destroy_renderer(renderer);
            tpxl_free_animation(&animation);
            return EXIT_FAILURE;
        }

        if (event.type == TPXL_EVENT_KEY) {
            if (event.key == TPXL_KEY_Q) {
                running = false;
            }
        }
        uint64_t now = get_time_ms();
        uint64_t delta = now - previous;
        previous = now;
        bool frame_changed = tpxl_update_animator(&animator, delta);

        if (frame_changed) {

            TpxlImage* frame = tpxl_get_animation_frame(&animator);
    
            result = tpxl_renderer_render(renderer, frame);

            if (result != TPXL_OK) {
                printf("\033[%uB", image_rows);
                printf("Error: %s\n", tpxl_result_to_string(result));
                tpxl_destroy_renderer(renderer);
                tpxl_free_animation(&animation);
                return EXIT_FAILURE;
            }
        }

        uint32_t remaining = animator.animation->delays[animator.current_frame] - animator.elapsed;

        sleep_ms(remaining);
    }

    printf("\033[%uB", image_rows + 1);
    fflush(stdout);

    tpxl_destroy_renderer(renderer);
    tpxl_free_animation(&animation);

    return EXIT_SUCCESS;
}
