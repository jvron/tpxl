#ifndef TPXL_ANIMATION_H
#define TPXL_ANIMATION_H

#include "tpxl/type.h"

TpxlResult tpxl_load_gif(const char* path, TpxlAnimation* animation);
void tpxl_free_animation(TpxlAnimation* animation);

#endif
