#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>

#include "tpxl/type.h"
#include "tpxl/image.h"
#include "tpxl/renderer.h"

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
                    "TPXL - Display images in the terminal\n"
                    "\n"
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

    result = tpxl_render(&image);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_free_image(&image);
        return EXIT_FAILURE;
    }

    tpxl_free_image(&image);

    return EXIT_SUCCESS;
}
