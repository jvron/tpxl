#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>

#include "tpxl/type.h"
#include "tpxl/image.h"
#include "tpxl/renderer.h"
#include "tpxl/animation.h"
#include "tpxl/file.h"
#include "tpxl/context.h"
#include "tpxl/terminal.h"
#include "tpxl/event.h"

uint64_t get_time_ms(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

void sleep_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}

int render_image(const char* file, bool print_info, TpxlContext* context) {
    TpxlResult result;

    TpxlImage image;
    result = tpxl_load_image(file, &image);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    if (print_info) {
        result = tpxl_print_image_info(&image);

        tpxl_free_image(&image);

        if (result != TPXL_OK) {
            printf("Error: %s\n", tpxl_result_to_string(result));
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    result = tpxl_update_context(context);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_image(&image);
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_viewport(context, image.width, image.height);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_image(&image);
        return EXIT_FAILURE;
    }

    result = tpxl_render(context, &image, TPXL_MEDIA_STILL);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_image(&image);
        return EXIT_FAILURE;
    }
    tpxl_free_image(&image);

    return EXIT_SUCCESS;
}

int render_gif(const char* path, TpxlContext* context) {

    TpxlResult result;

    TpxlAnimation animation; 
    result = tpxl_load_gif(path, &animation);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    result = tpxl_update_context(context);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_animation(&animation);
        return EXIT_FAILURE;
    }

    result = tpxl_update_context_viewport(context, animation.frames[0].width, animation.frames[0].height);

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
            printf("Error: %s\n", tpxl_result_to_string(result));
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

        bool frame_changed = tpxl_update_animation(&animator, delta);

        if (frame_changed) {

            TpxlImage* frame = tpxl_get_animation_frame(&animator);
    
            result = tpxl_render(context, frame, TPXL_MEDIA_ANIMATED);

            if (result != TPXL_OK) {
                printf("Error: %s\n", tpxl_result_to_string(result));
                tpxl_free_animation(&animation);
                return EXIT_FAILURE;
            }
    
            fflush(stdout);
        }

        uint32_t remaining = animator.animation->delays[animator.current_frame] - animator.elapsed;

        sleep_ms(remaining);
    }

    printf("\033[%uB", image_rows + 1);
    fflush(stdout);

    tpxl_free_animation(&animation);

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {

    bool print_info = false;

    static struct option options[] = {
        {"info", no_argument, NULL, 'i'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "ih", options, NULL)) != -1) {

        switch (opt) {
            case 'i':
                print_info = true;
                break;
            case 'h':
                printf(
                    "Usage:\n"
                    "    tpxl [OPTIONS] <file>\n"
                    "\n"
                    "Options:\n"
                    "    -i, --info       Print image information\n"
                    "    -h, --help       Show this help message\n"
                    "    -V, --version    Show version information\n"
                    "\n"
                    "Arguments:\n"
                    "    <file>           Image file to display\n"
                    "\n"
                    "Examples:\n"
                    "    tpxl image.png\n"
                    "    tpxl --info image.png\n"
                    "    tpxl -h\n"
                );
                return EXIT_SUCCESS;
            
            default:
                printf("Error: invalid option\n");
                return EXIT_FAILURE;
        }
    }

    const char* file = NULL;

    if (argc - optind == 1) {
        file = argv[optind];
    }
    else {
        printf("Usage: tpxl [OPTIONS] <file>\n");
        return EXIT_FAILURE;
    }
    
    TpxlContext context;

    TpxlResult result = tpxl_init_context(&context);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }
    
    result = tpxl_context_set_backend(&context, TPXL_BACKEND_KITTY);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    result = tpxl_context_set_scale_mode(&context, TPXL_SCALE_FIT);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    result = tpxl_context_set_alignment(&context, TPXL_ALIGN_START, TPXL_ALIGN_START);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    TpxlFileType file_type = tpxl_detect_file_type(file);

    int exit_code = 0;

    switch(file_type) {

        case TPXL_FILE_UNKNOWN:
            printf("Error: %s", tpxl_result_to_string(TPXL_INVALID_FILE));
            return EXIT_FAILURE;

        case TPXL_FILE_JPEG:
        case TPXL_FILE_PNG:
            exit_code = render_image(file, print_info, &context);
            break;

        case TPXL_FILE_GIF:
            exit_code = render_gif(file, &context);
            break;
        
        default:
            printf("Error: unsupported file type\n");
            return EXIT_FAILURE;
    }

    result = tpxl_shutdown_terminal(&context.terminal);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    return exit_code;
}
