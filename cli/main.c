#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "tpxl/audio.h"
#include "tpxl/type.h"
#include "tpxl/file.h"
#include "tpxl/context.h"
#include "tpxl/terminal.h"

#include "cli.h"

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
                    "    -i, --info       Print media information\n"
                    "    -h, --help       Show this help message\n"
                    "    -V, --version    Show version information\n"
                    "\n"
                    "Arguments:\n"
                    "    <file>           Media file to open\n"
                    "\n"
                    "Examples:\n"
                    "    tpxl image.png\n"
                    "    tpxl animation.gif\n"
                    "    tpxl video.mp4\n"
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

    TpxlFileType file_type = tpxl_detect_file_type(file);

    if (file_type == TPXL_FILE_UNKNOWN) {
        printf("Error: %s", tpxl_result_to_string(TPXL_INVALID_FILE));
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

    int exit_code = 0;

    switch(file_type) {
        case TPXL_FILE_JPEG:
        case TPXL_FILE_PNG:
            exit_code = display_image(file, print_info, &context);
            break;

        case TPXL_FILE_GIF:
            exit_code = display_gif(file, &context, print_info);
            break;
        case TPXL_FILE_VIDEO:
            exit_code = play_video(file, &context);
            break;
        case TPXL_FILE_AUDIO:
            exit_code = play_audio(file);
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
