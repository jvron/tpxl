#ifndef TPXL_ANIMATION_H
#define TPXL_ANIMATION_H

#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/renderer.h"

/**
 * Represents an animation player.
 *
 * The implementation is opaque and managed by the animation player
 * functions.
 */
typedef struct TpxlAnimationPlayerImp TpxlAnimationPlayer;

/**
 * Represents a single frame of an animation.
 *
 * The frame contains the image data, its display duration, and its
 * frame ID.
 */
typedef struct {
    TpxlImage frame;
    uint32_t delay;
    uint32_t frame_id;
} TpxlAnimationFrame;


/**
 * Represents a loaded animation.
 *
 * Stores the original animation dimensions, output dimensions, pixel
 * format, and collection of animation frames.
 */
typedef struct {
    uint32_t width;
    uint32_t height;

    uint32_t output_width;
    uint32_t output_height;
    TpxlFormat format;

    TpxlAnimationFrame* frames;
    size_t frame_count;
    size_t frame_capacity;
}  TpxlAnimation;

/**
 * Loads an animated image from a file.
 *
 * On success, the animation structure is populated with the animation's
 * dimensions, pixel format, and decoded frames.
 *
 * Each frame contains a complete image representing the animation at that
 * point in time. Frames use RGB or RGBA format, depending on whether
 * transparency is required.
 *
 * The loaded animation data must be released using
 * tpxl_animation_free().
 *
 * @param path Path to the animation file.
 * @param animation Animation structure to populate.
 *
 * @return TPXL_OK on success.
 * @return TPXL_LOAD_FAILED if the animation cannot be loaded or decoded.
 * @return TPXL_OUT_OF_MEMORY if memory allocation fails.
 * @return TPXL_INVALID_ARGUMENT if path or animation is NULL.
 */
TpxlResult tpxl_animation_load(const char* path, TpxlAnimation* animation);

/**
 * Resizes all frames of an animation to the specified dimensions.
 *
 * On success, the output dimensions of the animation are updated.
 * The original animation dimensions remain unchanged.
 *
 * @param animation Animation whose frames are to be resized.
 * @param output_width Desired output width in pixels.
 * @param output_height Desired output height in pixels.
 *
 * @return TPXL_OK on success.
 * @return TPXL_RESIZE_FAILED if a frame cannot be resized.
 * @return TPXL_INVALID_FORMAT if a frame has an unsupported pixel format.
 * @return TPXL_OUT_OF_MEMORY if memory allocation fails.
 * @return TPXL_INVALID_ARGUMENT if animation is NULL.
 */
TpxlResult tpxl_animation_resize(TpxlAnimation* animation, uint32_t output_width, uint32_t output_height);

/**
 * Prints information about an animation to standard output.
 *
 * The information includes the original width, original height,
 * pixel format, and number of frames.
 *
 * @param animation Animation whose information is to be printed.
 *
 * @return TPXL_OK on success.
 * @return TPXL_INVALID_ARGUMENT if animation is NULL.
 */
TpxlResult tpxl_animation_print_info(const TpxlAnimation* animation);

/**
 * Frees the memory owned by an animation.
 *
 * Frees the pixel data of each frame and the frame array, then resets
 * the animation structure to zero.
 *
 * Calling this function with a NULL pointer has no effect.
 *
 * @param animation Animation whose resources are to be freed.
 */
void tpxl_animation_free(TpxlAnimation* animation);

/**
 * Creates an animation player.
 *
 * Creates a player associated with the specified renderer and animation.
 * Each animation frame is uploaded to the renderer during creation.
 *
 * The animation and renderer must remain valid for the lifetime of
 * the player.
 *
 * On success, the player pointer is populated with the newly created
 * player. The player must eventually be released using
 * tpxl_animation_player_destroy().
 *
 * @param player Output pointer receiving the created animation player.
 * @param renderer Renderer used to display the animation.
 * @param animation Animation to play.
 *
 * @return TPXL_OK on success.
 * @return TPXL_PLAYER_CREATION_FAILED if an animation frame cannot be
 *         uploaded to the renderer.
 * @return TPXL_INVALID_ARGUMENT if player, renderer, or animation is NULL.
 * @return TPXL_OUT_OF_MEMORY if memory allocation fails.
 */
TpxlResult tpxl_animation_player_create(TpxlAnimationPlayer** player, TpxlRenderer* renderer, TpxlAnimation* animation);

/**
 * Starts playing an animation at the specified terminal position.
 *
 * Playback runs on a separate thread and continues until the player
 * is destroyed.
 *
 * Calling this function on a player whose playback thread has already
 * been created has no effect.
 *
 * Row and column positions are zero-based.
 *
 * @param player Animation player to start.
 * @param row Destination row.
 * @param column Destination column.
 *
 * @return TPXL_OK on success.
 * @return TPXL_THREAD_ERROR if the playback thread cannot be created.
 * @return TPXL_INVALID_ARGUMENT if player is NULL.
 */
TpxlResult tpxl_animation_play(TpxlAnimationPlayer* player, uint32_t row, uint32_t column);

/**
 * Stops playback and destroys an animation player.
 *
 * Signals the playback thread to stop, waits for it to finish, releases
 * the renderer resources associated with the animation frames, and
 * frees the player.
 *
 * Sets the player pointer to NULL after destruction.
 *
 * Calling this function with a NULL pointer or a pointer to a NULL
 * player has no effect.
 *
 * The associated animation and renderer must remain valid until the
 * player has been destroyed.
 *
 * @param player Pointer to the animation player to destroy.
 */
void tpxl_animation_player_destroy(TpxlAnimationPlayer** player);

#endif