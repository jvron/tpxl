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

    result = tpxl_image_load(file, &image);
    if (result != TPXL_OK) goto error;

    if (print_info) {
        result = tpxl_image_print_info(&image);
        if (result != TPXL_OK) goto error;

        goto cleanup;
    }

    const uint32_t margin = 64;

    uint32_t area_width = terminal->pixel_width > margin ? terminal->pixel_width - margin : terminal->pixel_width;
    uint32_t area_height = terminal->pixel_height > margin ? terminal->pixel_height - margin : terminal->pixel_height;

    uint32_t output_width, output_height;

    // Downscale only when needed.
    if (image.width > area_width || image.height > area_height) {
        result = tpxl_scale_fit(
            area_width,
            area_height,
            image.width, 
            image.height, 
            &output_width,
            &output_height
        );
        if (result != TPXL_OK) goto error;
    
        if (output_width != image.width || output_height != image.height) {
            result = tpxl_image_resize(&image, output_width, output_height);
            if (result != TPXL_OK) goto error;
        }
    } else {
        output_width = image.width;
        output_height = image.height;
    }

    uint32_t image_rows = (output_height + terminal->cell_height - 1) / terminal->cell_height;

    result = tpxl_terminal_ensure_rows(terminal, image_rows);
    if (result != TPXL_OK) goto error;

    TpxlRendererConfig config;
    config.backend = backend;
    config.media_type = TPXL_MEDIA_IMAGE;
    config.terminal = terminal;
    config.render_width = output_width;
    config.render_height = output_height;
    config.format = image.format;
    
    result = tpxl_renderer_create(&renderer, &config);
    if (result != TPXL_OK) goto error;

    result = tpxl_renderer_direct_render(renderer, &image, terminal->cursor_row, terminal->cursor_column);
    if (result != TPXL_OK) goto error;

    goto cleanup;

    error:
        printf("Error: %s\n", tpxl_result_to_string(result));
        exit_code = EXIT_FAILURE;

    cleanup:
        fputc('\n', stdout);
        tpxl_image_free(&image);
        tpxl_renderer_destroy(&renderer);

    return exit_code;
}
