#ifndef TPXL_IMAGE_H
#define TPXL_IMAGE_H

#include "type.h"

TpxlResult tpxl_load_image(const char* file, TpxlImage* image);
TpxlResult tpxl_resize_image(TpxlImage* image, uint32_t output_width, uint32_t output_height);
void tpxl_free_image(TpxlImage* image);
void tpxl_free_frame(TpxlImage* frame);
TpxlResult tpxl_print_image_info(TpxlImage* image);

#endif
