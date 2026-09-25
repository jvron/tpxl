#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "tpxl/type.h"
#include "tpxl/file.h"
#include "tpxl/terminal.h"
#include "tpxl/renderer.h"

#include "cli.h"

int main(int argc, char* argv[]) {
    
    bool print_info = false;

    static struct option options[] = {
        {"info", no_argument, NULL, 'i'},
        {"help", no_argument, NULL, 'h'},
        {"backend", required_argument, NULL, 'b'},
        {NULL, 0, NULL, 0}
    };

    TpxlBackend backend = TPXL_BACKEND_AUTO;

    int opt;
    while ((opt = getopt_long(argc, argv, "ib:h", options, NULL)) != -1) {

        switch (opt) {
            case 'i':
                print_info = true;
                break;

            case 'b':
                if (strcmp(optarg, "kitty") == 0) {
                    backend = TPXL_BACKEND_KITTY;
                } else if (strcmp(optarg, "sixel") == 0) {
                    backend = TPXL_BACKEND_SIXEL;
                } else if (strcmp(optarg, "auto") == 0) {
                    backend = TPXL_BACKEND_AUTO;
                } else {
                    printf("Error: invalid backend\n");
                    return EXIT_FAILURE;
                }
                break;

            case 'h':
                printf(
                    "Usage:\n"
                    "    tpxl [OPTIONS] <file>\n"
                    "\n"
                    "Options:\n"
                    "    -i, --info           Print media information\n"
                    "    -b, --backend <name> Select rendering backend (auto, kitty, sixel)\n"
                    "    -h, --help           Show this help message\n"
                    "    -V, --version        Show version information\n"
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
    } else {
        printf("Usage: tpxl [OPTIONS] <file>\n");
        return EXIT_FAILURE;
    }

    TpxlFileType file_type = tpxl_detect_file_type(file);

    if (file_type == TPXL_FILE_UNKNOWN) {
        printf("Error: %s", tpxl_result_to_string(TPXL_INVALID_FILE));
        return EXIT_FAILURE;
    }
    
    TpxlTerminal terminal = {0};

    TpxlResult result = tpxl_init_terminal(&terminal);
    if (result != TPXL_OK) goto error;

    result = tpxl_query_terminal(&terminal);
    if (result != TPXL_OK) goto error;

    printf("\033[?25l");

    int exit_code = 0;

    switch(file_type) {
        case TPXL_FILE_JPEG:
        case TPXL_FILE_PNG:
            exit_code = display_image(file, &terminal,backend, print_info);
            break;
        case TPXL_FILE_GIF:
            exit_code = display_gif(file, &terminal, backend, print_info);
            break;
        case TPXL_FILE_VIDEO:
            exit_code = play_video(file, &terminal, backend);
            break;
        case TPXL_FILE_AUDIO:
            exit_code = play_audio(file);
            break;
        
        default:
            printf("Error: unsupported file type\n");
            return EXIT_FAILURE;
    }

    printf("\033[?25h");

    result = tpxl_shutdown_terminal(&terminal);
    if (result != TPXL_OK) goto error;

    return exit_code;

    error:
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
}
