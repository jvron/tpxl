#ifndef TPXL_IMAGE_H
#define TPXL_IMAGE_H

#include "type.h"

TpxlResult tpxl_load_image(const char* file, TpxlImage* image);
void tpxl_free_image(TpxlImage* image);

#endif
