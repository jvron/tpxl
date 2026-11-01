#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include <libbase64.h>

#include "base64.h"

size_t tpxl_base64_encoded_size(size_t length) {
    return 4 * ((length + 2) / 3);
}

void tpxl_base64_encode(const uint8_t* src, size_t srclen, char* output, size_t output_capacity, size_t* output_length) {

    assert(src && output && output_length);

    assert(output_capacity >= tpxl_base64_encoded_size(srclen));

    base64_encode((const char*)src, srclen, output, output_length, 0);
}
