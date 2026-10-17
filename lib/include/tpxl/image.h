#ifndef TPXL_IMAGE_H
#define TPXL_IMAGE_H

#include "type.h"

/**
 * Loads an image from a file.
 *
 * On success, the TpxlImage structure is populated with the image's
 * dimensions, pixel data, and pixel format.
 *
 * The loaded pixel data must be released using tpxl_image_free().
 *
 * @param file Path to the image file.
 * @param image Image structure to populate.
 *
 * @return TPXL_OK on success.
 * @return TPXL_LOAD_FAILED if the image cannot be loaded.
 * @return TPXL_UNSUPPORTED_FORMAT if the image format is unsupported.
 * @return TPXL_INVALID_ARGUMENT if file or image is NULL.
 */
TpxlResult tpxl_image_load(const char* file, TpxlImage* image);

/**
 * Resizes an image to the specified dimensions.
 *
 * On success, the image's pixel data is replaced with the resized pixel
 * data, and its dimensions are updated. The original pixel data is freed.
 *
 * The image's pixel format is preserved.
 *
 * @param image Image to resize.
 * @param output_width Desired output width in pixels.
 * @param output_height Desired output height in pixels.
 *
 * @return TPXL_OK on success.
 * @return TPXL_RESIZE_FAILED if the image cannot be resized.
 * @return TPXL_INVALID_FORMAT if the image's pixel format is unsupported.
 * @return TPXL_OUT_OF_MEMORY if memory allocation fails.
 * @return TPXL_INVALID_ARGUMENT if image is NULL.
 */
TpxlResult tpxl_image_resize(TpxlImage* image, uint32_t output_width, uint32_t output_height);

/**
 * Frees the pixel data owned by an image.
 *
 * After freeing the pixel data, the image structure is reset to zero.
 * Calling this function with a NULL pointer has no effect.
 *
 * Use this function for images whose pixel data was allocated by
 * tpxl_image_load() or is otherwise compatible with stbi_image_free().
 *
 * @param image Image whose pixel data is to be freed.
 */
void tpxl_image_free(TpxlImage* image);

/**
 * Frees the pixel data owned by a frame.
 *
 * After freeing the pixel data, the frame structure is reset to zero.
 * Calling this function with a NULL pointer has no effect.
 *
 * The pixel data is freed using the C standard library function free().
 *
 * @param frame Frame whose pixel data is to be freed.
 */
void tpxl_frame_free(TpxlImage* frame);

/**
 * Prints information about an image to standard output.
 *
 * The information includes the image's width, height, pixel format,
 * and number of color channels.
 *
 * @param image Image whose information is to be printed.
 *
 * @return TPXL_OK on success.
 * @return TPXL_INVALID_ARGUMENT if image is NULL.
 */
TpxlResult tpxl_image_print_info(const TpxlImage* image);

#endif
