#ifndef TPXL_CLI_H
#define TPXL_CLI_H

#include "tpxl/renderer.h"
#include "tpxl/terminal.h"

int display_image(const char* file, TpxlTerminal* terminal, TpxlBackend backend, bool print_info);
int display_gif(const char* path, TpxlTerminal* terminal, TpxlBackend backend, bool print_info);
int play_video(const char* path, TpxlTerminal* terminal, TpxlBackend backend);
int play_audio(const char* path);

#endif

