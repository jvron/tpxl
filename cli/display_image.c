#include <stdio.h>
#include <stdlib.h>

#include "tpxl/image.h"
#include "tpxl/renderer.h"
#include "tpxl/terminal.h"
#include "tpxl/type.h"
#include "tpxl/util.h"

#include "cli.h"

int display_image(const char* file, TpxlTerminal* terminal, TpxlBackend backend, bool print_info) {
    
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

    uint32_t area_width = terminal->pixel_width;
    uint32_t area_height = terminal->pixel_height * terminal->cell_width / terminal->cell_height;

    uint32_t output_width, output_height;
    result = tpxl_scale_fit(
        area_width,
        area_height,
        image.width, 
        image.height, 
        &output_width,
        &output_height
    );
    if (result != TPXL_OK) goto error;

    result = tpxl_resize_image(&image, output_width, output_height);
    if (result != TPXL_OK) goto error;

    TpxlRendererConfig config;
    config.backend = backend;
    config.media_type = TPXL_MEDIA_IMAGE;
    config.terminal = terminal;
    config.render_width = output_width;
    config.render_height = output_height;
    config.format = image.format;
    
    result = tpxl_create_renderer(&renderer, &config);
    if (result != TPXL_OK) goto error;

    result = tpxl_renderer_direct_render(renderer, &image, terminal->cursor_row, terminal->cursor_column);
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
