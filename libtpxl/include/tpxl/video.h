#ifndef TPXL_VIDEO_H
#define TPXL_VIDEO_H

#include <stdint.h>

#include "tpxl/type.h"

typedef struct TpxlVideoImp TpxlVideo;

TpxlResult tpxl_open_video(const char* path, TpxlVideo** video);
void tpxl_close_video(TpxlVideo* video);

#endif
