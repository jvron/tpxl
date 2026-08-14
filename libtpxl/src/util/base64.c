#include <stdint.h>
#include <stddef.h>
#include <libbase64.h>

#include "base64.h"
#include "tpxl/type.h"

size_t tpxl_base64_encoded_size(size_t length) {
    return 4 * ((length + 2) / 3);
}

TpxlResult tpxl_base64_encode(const uint8_t* src, size_t srclen, char* output, size_t* output_length) {

    if (!src || !output) {
        return TPXL_ERROR;
    }

    base64_encode((const char*)src, srclen, output, output_length, 0);

    return TPXL_OK;
}
