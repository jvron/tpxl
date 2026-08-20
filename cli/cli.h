#ifndef TPXL_CLI_H
#define TPXL_CLI_H

#include "tpxl/context.h"

int display_image(const char* file, bool print_info, TpxlContext* context);
int display_gif(const char* path, TpxlContext* context, bool print_info);
int play_video(const char* path, TpxlContext* context);

#endif

