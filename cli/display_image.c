#include <stdio.h>
#include <stdlib.h>

#include "tpxl/image.h"
#include "tpxl/renderer.h"

#include "cli.h"

int display_image(const char* file, bool print_info, TpxlContext* context) {
    
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

    result = tpxl_update_context_terminal(context);

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

    TpxlRenderer* renderer = NULL;
    result = tpxl_create_renderer(&renderer, context, image.width, image.height, image.format, TPXL_MEDIA_STILL);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_image(&image);
        return EXIT_FAILURE;
    }

    result = tpxl_renderer_render(renderer, &image);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_destroy_renderer(renderer);
        tpxl_free_image(&image);
        return EXIT_FAILURE;
    }

    tpxl_free_image(&image);
    tpxl_destroy_renderer(renderer);

    return EXIT_SUCCESS;
}
