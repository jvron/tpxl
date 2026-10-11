#ifndef TPXL_IMAGE_H
#define TPXL_IMAGE_H

#include "type.h"

TpxlResult tpxl_image_load(const char* file, TpxlImage* image);
TpxlResult tpxl_image_resize(TpxlImage* image, uint32_t output_width, uint32_t output_height);
void tpxl_image_free(TpxlImage* image);
void tpxl_frame_free(TpxlImage* frame);
TpxlResult tpxl_image_print_info(const TpxlImage* image);

#endif
