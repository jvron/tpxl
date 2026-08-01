#include <stdint.h>
#include <stddef.h>

#include "base64.h"
#include "tpxl/type.h"

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t tpxl_base64_encoded_size(size_t length) {
    return 4 * ((length + 2) / 3) + 1;
}

TpxlResult tpxl_base64_encode(const uint8_t* data, size_t length, char* encoded) {

    if (!data || !encoded) {
        return TPXL_ERROR;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < length) {

        size_t remaining = length - i;

        uint32_t a = data[i++];
        uint32_t b = remaining > 1 ? data[i++] : 0;
        uint32_t c = remaining > 2 ? data[i++] : 0;

        uint32_t triple = (a << 16) | (b << 8) | c;

        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];

        encoded[j++] = remaining > 1 ? base64_table[(triple >> 6) & 0x3F] : '=';
        encoded[j++] = remaining > 2 ? base64_table[triple & 0x3F] : '=';
    }

    encoded[j] = '\0';

    return TPXL_OK;
}
