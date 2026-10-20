#ifndef TPXL_RENDERER_H
#define TPXL_RENDERER_H

#include "tpxl/terminal.h"

/**
 * Represents a renderer instance.
 */
typedef struct TpxlRendererImp TpxlRenderer;

/**
 * Defines how the terminal cursor is handled after rendering.
 */
typedef enum {
    TPXL_CURSOR_PRESERVE,
    TPXL_CURSOR_ADVANCE,
} TpxlCursorPolicy;

/**
 * Specifies the graphics protocol used to render images in the terminal.
 */
typedef enum {
    TPXL_BACKEND_AUTO = 0,
    TPXL_BACKEND_KITTY,
    TPXL_BACKEND_SIXEL,
    TPXL_BACKEND_ITERM2,
    TPXL_BACKEND_COUNT,
} TpxlBackend;

/**
 * Configuration used to create a renderer.
 */
typedef struct {
    TpxlTerminal* terminal;

    TpxlBackend backend;
    TpxlMediaType media_type;
    
    TpxlFormat format;
    uint32_t render_width;
    uint32_t render_height;
} TpxlRendererConfig;

/**
 * Creates a renderer using the specified configuration.
 *
 * On success, the renderer is initialized with the selected backend,
 * media policy, terminal, rendering dimensions, and pixel format.
 *
 * The renderer must be released using tpxl_renderer_destroy().
 *
 * @param renderer Output pointer receiving the created renderer.
 * @param config Configuration used to initialize the renderer.
 *
 * @return TPXL_OK on success.
 * @return TPXL_OUT_OF_MEMORY if memory allocation fails.
 * @return TPXL_INVALID_BACKEND if the specified backend is invalid or
 *         unsupported by this function.
 * @return TPXL_INVALID_ARGUMENT if renderer, config, or config->terminal
 *         is NULL.
 * @return The error returned by the selected backend if initialization
 *         fails.
 * @note On failure, *renderer is set to NULL if renderer != NULL.
 */
TpxlResult tpxl_renderer_create(TpxlRenderer** renderer, TpxlRendererConfig* config);

/**
 * Updates the renderer's output dimensions and pixel format.
 *
 * @param renderer Renderer to update.
 * @param render_width Desired rendering width in pixels.
 * @param render_height Desired rendering height in pixels.
 * @param format Pixel format to use for rendering.
 *
 * @return TPXL_OK on success.
 * @return TPXL_INVALID_ARGUMENT if renderer is NULL.
 * @return TPXL_INVALID_BACKEND if the renderer's backend is invalid.
 * @return The error returned by the selected backend if the update fails.
 */
TpxlResult tpxl_renderer_update_frame(TpxlRenderer* renderer, uint32_t render_width, uint32_t render_height, TpxlFormat format);

/**
 * Updates the renderer's media handling policy.
 *
 * @param renderer Renderer to update.
 * @param media_type Media type to use.
 *
 * @return TPXL_OK on success.
 * @return TPXL_INVALID_ARGUMENT if renderer is NULL.
 * @return TPXL_INVALID_BACKEND if the renderer's backend is invalid.
 * @return The error returned by the selected backend if the update fails.
 */
TpxlResult tpxl_renderer_update_media_policy(TpxlRenderer* renderer, TpxlMediaType media_type);

/**
 * Processes and renders an image directly at the specified terminal position.
 *
 * Row and column positions are one-based.
 *
 * @param renderer Renderer to use.
 * @param frame Image to render.
 * @param row Destination row.
 * @param column Destination column.
 *
 * @return TPXL_OK on success.
 * @return TPXL_INVALID_ARGUMENT if renderer or frame is NULL, or if
 *         frame->pixels is NULL.
 * @return TPXL_INVALID_BACKEND if the renderer's backend is invalid.
 * @return The error returned by the selected backend if rendering fails.
 */
TpxlResult tpxl_renderer_direct_render(TpxlRenderer* renderer, TpxlImage* frame, uint32_t row, uint32_t column);

/**
 * Processes an image for rendering and stores it internally.
 *
 * The image is identified by frame_id and can subsequently be displayed
 * using tpxl_renderer_display().
 *
 * @param renderer Renderer to use.
 * @param frame Image to upload.
 * @param frame_id Identifier assigned to the uploaded image. Must not be zero.
 *
 * @return TPXL_OK on success.
  * @return TPXL_INVALID_ARGUMENT if renderer or frame is NULL, frame->pixels
 *         is NULL, or frame_id is zero.
 * @return TPXL_INVALID_BACKEND if the renderer's backend is invalid.
 * @return The error returned by the selected backend if the upload fails.
 */
TpxlResult tpxl_renderer_upload(TpxlRenderer* renderer, TpxlImage* frame, uint32_t frame_id);

/**
 * Displays a previously uploaded image at the specified terminal position.
 *
 * The image is identified by frame_id.
 * Row and column positions are one-based.
 *
 * @param renderer Renderer to use.
 * @param frame_id Identifier of the uploaded image. Must not be zero.
 * @param row Destination row.
 * @param column Destination column.
 *
 * @return TPXL_OK on success.
 * @return TPXL_INVALID_ARGUMENT if renderer is NULL or frame_id is zero.
 * @return TPXL_INVALID_BACKEND if the renderer's backend is invalid.
 * @return The error returned by the selected backend if displaying the
 *         image fails.
 */
TpxlResult tpxl_renderer_display(TpxlRenderer* renderer, uint32_t frame_id, uint32_t row, uint32_t column);

/**
 * Gets the backend used by a renderer.
 *
 * @param renderer Renderer to query. Must not be NULL.
 *
 * @return The backend used by the renderer.
 *
 * @note Passing NULL triggers an assertion failure.
 */
TpxlBackend tpxl_renderer_get_backend(TpxlRenderer* renderer);

/**
 * Deletes the terminal placement associated with an uploaded image.
 *
 * This removes the image's placement without deleting the
 * uploaded image data.
 *
 * @param renderer Renderer to use.
 * @param frame_id Identifier of the image whose placement is to be deleted. Must not be zero.
 *
 * @note If renderer is NULL or frame_id is zero, the function has no effect.
 */
void tpxl_renderer_delete_placement(TpxlRenderer* renderer, uint32_t frame_id);

/**
 * Deletes the data associated with an uploaded image.
 *
 * @param renderer Renderer to use.
 * @param frame_id Identifier of the image whose data is to be deleted. Must not be zero.
 *
 * @note If renderer is NULL or frame_id is zero, the function has no effect.
 */
void tpxl_renderer_delete_data(TpxlRenderer* renderer, uint32_t frame_id);

/**
 * Destroys a renderer and releases its resources.
 *
 * The renderer pointer is set to NULL after destruction.
 * Calling this function with a NULL pointer or a pointer to NULL has
 * no effect.
 *
 * @param renderer Pointer to the renderer to destroy.
 */
void tpxl_renderer_destroy(TpxlRenderer** renderer);

#endif