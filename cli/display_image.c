#include <stdio.h>
#include <stdlib.h>

#include "tpxl/image.h"
#include "tpxl/renderer.h"

#include "cli.h"

int display_image(const char* file, TpxlContext* context, bool print_info) {
    
    int exit_code = EXIT_SUCCESS;
    TpxlResult result = TPXL_OK;

    TpxlImage image = {0};
    TpxlRenderer* renderer = NULL;

    result = tpxl_load_image(file, &image);
    if (result != TPXL_OK) goto error;

    if (print_info) {
        result = tpxl_print_image_info(&image);
        if (result != TPXL_OK) goto error;

        goto cleanup;
    }

    result = tpxl_update_context_terminal(context);
    if (result != TPXL_OK) goto error;

    result = tpxl_update_context_viewport(context, image.width, image.height);
    if (result != TPXL_OK) goto error;
    
    result = tpxl_create_renderer(&renderer, context, image.width, image.height, image.format, TPXL_MEDIA_IMAGE);
    if (result != TPXL_OK) goto error;

    result = tpxl_renderer_render(renderer, &image);
    if (result != TPXL_OK) goto error;

    goto cleanup;

    error:
    printf("Error: %s\n", tpxl_result_to_string(result));
    exit_code = EXIT_FAILURE;

    cleanup:
        tpxl_free_image(&image);
        tpxl_destroy_renderer(&renderer);

    return exit_code;
}
