#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gif_lib.h>

#include "tpxl/animation.h"
#include "tpxl/image.h"
#include "tpxl/type.h"

typedef struct {
    uint32_t delay;
    int transparent_index;
    uint8_t disposal;
} GifGraphicsControl;

static void tpxl_blit(TpxlImage* dst, const TpxlImage* src, uint32_t left, uint32_t top) {

    for (uint32_t y = 0; y < src->height; y++) {
        for (uint32_t x = 0; x < src->width; x++) {

            size_t src_pixel = (y * src->width + x) * 4;

            size_t dst_pixel = ((top + y) * dst->width + (left + x)) * 4;

            if (src->pixels[src_pixel + 3] == 0) {
                continue; // skip transparent pixels
            }

            dst->pixels[dst_pixel + 0] = src->pixels[src_pixel + 0];
            dst->pixels[dst_pixel + 1] = src->pixels[src_pixel + 1];
            dst->pixels[dst_pixel + 2] = src->pixels[src_pixel + 2];
            dst->pixels[dst_pixel + 3] = src->pixels[src_pixel + 3];
        }
    }
}

static void tpxl_parse_graphics_control(const SavedImage* frame, GifGraphicsControl* gce) {

    // defaults
    gce->delay = 100;
    gce->transparent_index = -1;
    gce->disposal = 0;

    for (int i = 0; i < frame->ExtensionBlockCount; i++) {

        ExtensionBlock* extension = &frame->ExtensionBlocks[i];

        if (extension->Function != GRAPHICS_EXT_FUNC_CODE) {
            continue;
        }

        if (extension->ByteCount < 4) {
            continue;
        }

        uint8_t packed = extension->Bytes[0];

        gce->disposal = (packed >> 2) & 0x07;

        bool has_transparency = packed & 0x01;

        if (has_transparency) {
            gce->transparent_index = extension->Bytes[3];
        }

        uint16_t delay = extension->Bytes[1] | (extension->Bytes[2] << 8);
        gce->delay = delay * 10;

        return;
    }
}

static void tpxl_apply_disposal(TpxlImage* canvas, const SavedImage* frame, GifGraphicsControl gce, const uint8_t* previous_canvas) {

    switch (gce.disposal) {
        case 0:
        case 1:
            // do not dispose
            break;
        case 2:
            for (int y = 0; y < frame->ImageDesc.Height; y++) {
                for (int x = 0; x < frame->ImageDesc.Width; x++) {
                    
                    size_t pixel = ((frame->ImageDesc.Top + y) * canvas->width + (frame->ImageDesc.Left + x)) * 4;
                    
                    canvas->pixels[pixel + 0] = 0;
                    canvas->pixels[pixel + 1] = 0;
                    canvas->pixels[pixel + 2] = 0;
                    canvas->pixels[pixel + 3] = 0;
                }
            }
            break;
        case 3:
            if (previous_canvas) {
                memcpy(canvas->pixels, previous_canvas, canvas->width * canvas->height * 4);
            }
            break;
    }
}

TpxlResult tpxl_load_animation(const char* path, TpxlAnimation* animation) {

    if (!path || !animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    GifFileType* gif = DGifOpenFileName(path, NULL);

    if (!gif) {
        return TPXL_ANIMATION_LOAD_FAILED;
    }

    // decode gif
    if (DGifSlurp(gif) != GIF_OK) {
        DGifCloseFile(gif, NULL);
        return TPXL_ANIMATION_LOAD_FAILED;
    }

    animation->width = gif->SWidth;
    animation->height = gif->SHeight;
    animation->output_width = animation->width;
    animation->output_height = animation->height;
    animation->format = TPXL_FORMAT_RGBA;

    animation->frame_count = gif->ImageCount;
    animation->frame_capacity = gif->ImageCount;
    
    animation->frames = calloc(animation->frame_capacity, sizeof(TpxlAnimationFrame));

    if (!animation->frames) {
        DGifCloseFile(gif, NULL);
        return TPXL_ANIMATION_LOAD_FAILED;
    }

    TpxlImage canvas;
    canvas.width = gif->SWidth;
    canvas.height = gif->SHeight;
    canvas.format = TPXL_FORMAT_RGBA;

    size_t canvas_size = canvas.width * canvas.height * 4;

    canvas.pixels = malloc(canvas_size);

    if (!canvas.pixels) {
        DGifCloseFile(gif, NULL);
        return TPXL_OUT_OF_MEMORY;
    }
    memset(canvas.pixels, 0, canvas_size);

    uint8_t* previous_canvas = malloc(canvas_size);

    if (!previous_canvas) {
        free(canvas.pixels);
        DGifCloseFile(gif, NULL);
        return TPXL_OUT_OF_MEMORY;
    }
    memset(previous_canvas, 0, canvas_size);

    TpxlResult result = TPXL_OK;

    for (size_t i = 0; i < gif->ImageCount; i++) {

        SavedImage* frame = &gif->SavedImages[i];

        GifGraphicsControl gce;
        tpxl_parse_graphics_control(frame, &gce);
       
        TpxlAnimationFrame* animation_frame = &animation->frames[i];
        animation_frame->frame_id = 0;
        animation_frame->delay = gce.delay;

        TpxlImage image = {
            .width = frame->ImageDesc.Width,
            .height = frame->ImageDesc.Height,
            .format = TPXL_FORMAT_RGBA,
            .pixels = NULL
        };

        size_t image_size = image.width * image.height * 4;
        image.pixels = malloc(image_size);

        if (!image.pixels) {
            result = TPXL_OUT_OF_MEMORY;
            break;
        }

        ColorMapObject* palette = frame->ImageDesc.ColorMap;

        if (!palette) {
            palette = gif->SColorMap;

            if (!palette) {
                free(image.pixels);
                result = TPXL_ANIMATION_LOAD_FAILED;
                break;
            }
        }

        for (uint32_t y = 0; y < image.height; y++) {
            for (uint32_t x = 0; x < image.width; x++) {

                uint8_t index = frame->RasterBits[y * image.width + x];

                GifColorType color = palette->Colors[index];

                size_t pixel = (y * image.width + x) * 4;

                image.pixels[pixel + 0] = color.Red;
                image.pixels[pixel + 1] = color.Green;
                image.pixels[pixel + 2] = color.Blue;

                if (index == gce.transparent_index && gce.transparent_index >= 0) {
                    image.pixels[pixel + 3] = 0;
                } else {                
                    image.pixels[pixel + 3] = 255;
                }
            }
        }

        if (gce.disposal == 3) {
            memcpy(previous_canvas, canvas.pixels, canvas_size);
        }

        tpxl_blit(&canvas, &image, frame->ImageDesc.Left, frame->ImageDesc.Top);

        animation_frame->frame.width = canvas.width;
        animation_frame->frame.height = canvas.height;
        animation_frame->frame.format = canvas.format;
        animation_frame->frame.pixels = malloc(canvas_size);

        if (!animation_frame->frame.pixels) {
            free(image.pixels);
            result = TPXL_OUT_OF_MEMORY;
            break;
        }

        memcpy(animation_frame->frame.pixels, canvas.pixels, canvas_size);

        if (gce.disposal == 3) {
            tpxl_apply_disposal(&canvas, frame, gce, previous_canvas);
        } else {
            tpxl_apply_disposal(&canvas, frame, gce, NULL);
        }

        free(image.pixels);
    }

    if (result != TPXL_OK) {

        for (size_t i = 0; i < animation->frame_count; i++) {
            tpxl_free_frame(&animation->frames[i].frame);
        }

        free(animation->frames);
        animation->frame_count = 0;
        animation->frame_capacity = 0;
    }

    free(canvas.pixels);
    free(previous_canvas);
    DGifCloseFile(gif, NULL);

    return result;
}

TpxlResult tpxl_resize_animation(TpxlAnimation* animation, uint32_t output_width, uint32_t output_height) {

    if (!animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    TpxlResult result = TPXL_OK;

    for (size_t i = 0; i < animation->frame_count; i++) {
        
        result = tpxl_resize_image(&animation->frames[i].frame, output_width, output_height);

        if (result != TPXL_OK) {
            return result;
        }
    }

    animation->output_width = output_width;
    animation->output_height = output_height;

    return result;
}

void tpxl_free_animation(TpxlAnimation* animation) {

    if (!animation) {
        return;
    }

    for (size_t i = 0; i < animation->frame_count; i++) {
        tpxl_free_frame(&animation->frames[i].frame);
    }

    free(animation->frames);

    *animation = (TpxlAnimation){0};
}

TpxlResult tpxl_print_animation_info(TpxlAnimation* animation) {

    if (!animation) {
        return TPXL_INVALID_ARGUMENT;
    }

    printf("Width: %d\n", animation->width);
    printf("Height: %d\n", animation->height);
    printf("Format: %s\n", tpxl_format_to_string(animation->format));
    printf("Frame count: %lu\n", animation->frame_count);

    return TPXL_OK;
}