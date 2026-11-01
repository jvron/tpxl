#ifndef TPXL_BASE64_H
#define TPXL_BASE64_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Calculates the Base64-encoded size of an input buffer.
 *
 * The returned size excludes the null terminator.
 *
 * @param length Input buffer size in bytes.
 *
 * @return Required output buffer size in bytes.
 */
size_t tpxl_base64_encoded_size(size_t length);

/**
 * @brief Encodes data into Base64.
 *
 * Writes the Base64-encoded data into the provided output buffer.
 * The caller is responsible for allocating sufficient output capacity.
 *
 * @param src Input data to encode.
 * @param srclen Input data size in bytes.
 * @param output Destination buffer.
 * @param output_capacity Destination buffer capacity in bytes.
 * @param output_len Output pointer receiving the encoded data length.
 *
 * @note The output is not null-terminated.
 * @note The source and destination pointers must be valid.
 * @note output_capacity must be at least the size returned by
 *       tpxl_base64_encoded_size().
 */
void tpxl_base64_encode(
    const uint8_t* src, 
    size_t srclen, 
    char* output, 
    size_t output_capacity, 
    size_t* output_len
);

#endif
