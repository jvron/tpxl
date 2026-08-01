#ifndef TPXL_BASE64_H
#define TPXL_BASE64_H

#include <stdint.h>
#include <stddef.h>

#include "tpxl/type.h"

size_t tpxl_base64_encoded_size(size_t length);
TpxlResult tpxl_base64_encode(const uint8_t* bytes, size_t length, char* output);

#endif
