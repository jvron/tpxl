#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "tpxl/type.h"
#include "sixel_image_map.h"

TpxlResult tpxl_init_sixel_image_map(TpxlSixelImageMap* image_map) {

    if (!image_map) {
        return TPXL_INVALID_ARGUMENT;
    }

    *image_map = (TpxlSixelImageMap){0};

    return TPXL_OK;
}

static uint32_t tpxl_hash_u32(uint32_t key) {
    return key * 2654435761u;
}

TpxlResult tpxl_sixel_image_map_insert(TpxlSixelImageMap* image_map, TpxlSixelImage* sixel_image, uint32_t frame_id) {

    if (!image_map || !sixel_image) {
        return TPXL_INVALID_ARGUMENT;
    }

    size_t index = tpxl_hash_u32(frame_id) % TPXL_SIXEL_IMAGE_MAP_CAPACITY;

    int deleted_index = -1;

    for (size_t probe = 0; probe < TPXL_SIXEL_IMAGE_MAP_CAPACITY; probe++) {

        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {

            if (deleted_index >= 0) {
                image_map->entries[deleted_index] = *sixel_image;
                image_map->states[deleted_index] = TPXL_ENTRY_OCCUPIED;
            } 
            else {
                image_map->entries[index] = *sixel_image;
                image_map->states[index] = TPXL_ENTRY_OCCUPIED;
            }
            image_map->count++;

            return TPXL_OK;
        } 
        else if (image_map->states[index] == TPXL_ENTRY_DELETED) {
            deleted_index = index;
        }
        else if (image_map->states[index] == TPXL_ENTRY_OCCUPIED && image_map->entries[index].id == frame_id) {
            // Update the entry
            image_map->entries[index] = *sixel_image;
            image_map->states[index] = TPXL_ENTRY_OCCUPIED;
            return TPXL_OK;
        }
        index = (index + 1) % TPXL_SIXEL_IMAGE_MAP_CAPACITY;
    }

    if (deleted_index >= 0) {
        image_map->entries[deleted_index] = *sixel_image;
        image_map->states[deleted_index] = TPXL_ENTRY_OCCUPIED;
        image_map->count++;
        return TPXL_OK;
    } 

    return TPXL_MAP_FULL;
}

TpxlResult tpxl_sixel_image_map_remove(TpxlSixelImageMap* image_map, uint32_t frame_id) {

    if (!image_map) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (image_map->count == 0) {
        return TPXL_MAP_EMPTY;
    }

    size_t index = tpxl_hash_u32(frame_id) % TPXL_SIXEL_IMAGE_MAP_CAPACITY;

    for (size_t probe = 0; probe < TPXL_SIXEL_IMAGE_MAP_CAPACITY; probe++) {
        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {
            return TPXL_MAP_NOT_FOUND;
        }
        if (image_map->states[index] == TPXL_ENTRY_OCCUPIED) {

            if (image_map->entries[index].id == frame_id) {
                image_map->states[index] = TPXL_ENTRY_DELETED;
                image_map->count--;
                return TPXL_OK;
            }
        }
        index = (index + 1) % TPXL_SIXEL_IMAGE_MAP_CAPACITY;
    }

    return TPXL_MAP_NOT_FOUND;
}

TpxlResult tpxl_get_sixel_image(TpxlSixelImageMap* image_map, uint32_t frame_id, TpxlSixelImage* out_sixel_image) {

    if (!image_map || !out_sixel_image) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (image_map->count == 0) {
        return TPXL_MAP_EMPTY;
    }

    size_t index = tpxl_hash_u32(frame_id) % TPXL_SIXEL_IMAGE_MAP_CAPACITY;

    for (size_t probe = 0; probe < TPXL_SIXEL_IMAGE_MAP_CAPACITY; probe++) {
        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {
            return TPXL_MAP_NOT_FOUND;
        }
        if (image_map->states[index] == TPXL_ENTRY_OCCUPIED) {

            if (image_map->entries[index].id == frame_id) {
                *out_sixel_image = image_map->entries[index];
                return TPXL_OK;
            }
        }
        index = (index + 1) % TPXL_SIXEL_IMAGE_MAP_CAPACITY;
    }

    return TPXL_MAP_NOT_FOUND;
}