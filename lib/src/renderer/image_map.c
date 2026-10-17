#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "tpxl/type.h"

#include "image_map.h"

TpxlResult tpxl_image_map_init(TpxlImageMap* image_map) {

    assert(image_map);

    *image_map = (TpxlImageMap){0};

    return TPXL_OK;
}

static uint32_t tpxl_hash_u32(uint32_t key) {
    return key * 2654435761u;
}

TpxlResult tpxl_image_map_insert(TpxlImageMap* image_map, TpxlImageMapEntry* entry, uint32_t frame_id) {

    assert(image_map && entry);

    size_t index = tpxl_hash_u32(frame_id) % TPXL_IMAGE_MAP_CAPACITY;

    int deleted_index = -1;

    for (size_t probe = 0; probe < TPXL_IMAGE_MAP_CAPACITY; probe++) {

        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {

            if (deleted_index >= 0) {
                image_map->entries[deleted_index] = *entry;
                image_map->states[deleted_index] = TPXL_ENTRY_OCCUPIED;
            } 
            else {
                image_map->entries[index] = *entry;
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
            image_map->entries[index] = *entry;
            image_map->states[index] = TPXL_ENTRY_OCCUPIED;
            return TPXL_OK;
        }
        index = (index + 1) % TPXL_IMAGE_MAP_CAPACITY;
    }

    if (deleted_index >= 0) {
        image_map->entries[deleted_index] = *entry;
        image_map->states[deleted_index] = TPXL_ENTRY_OCCUPIED;
        image_map->count++;
        return TPXL_OK;
    } 

    return TPXL_MAP_FULL;
}

TpxlResult tpxl_image_map_remove(TpxlImageMap* image_map, uint32_t frame_id) {

    assert(image_map);

    if (image_map->count == 0) {
        return TPXL_MAP_EMPTY;
    }

    size_t index = tpxl_hash_u32(frame_id) % TPXL_IMAGE_MAP_CAPACITY;

    for (size_t probe = 0; probe < TPXL_IMAGE_MAP_CAPACITY; probe++) {
        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {
            return TPXL_MAP_NOT_FOUND;
        }
        if (image_map->states[index] == TPXL_ENTRY_OCCUPIED) {

            if (image_map->entries[index].id == frame_id) {
                free(image_map->entries[index].data);
                image_map->entries[index].data = NULL;
                image_map->states[index] = TPXL_ENTRY_DELETED;
                image_map->count--;
                return TPXL_OK;
            }
        }
        index = (index + 1) % TPXL_IMAGE_MAP_CAPACITY;
    }

    return TPXL_MAP_NOT_FOUND;
}

TpxlResult tpxl_image_map_get(TpxlImageMap* image_map, uint32_t frame_id, TpxlImageMapEntry** out_entry) {

    assert(image_map && out_entry);

    if (image_map->count == 0) {
        return TPXL_MAP_EMPTY;
    }

    size_t index = tpxl_hash_u32(frame_id) % TPXL_IMAGE_MAP_CAPACITY;

    for (size_t probe = 0; probe < TPXL_IMAGE_MAP_CAPACITY; probe++) {
        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {
            return TPXL_MAP_NOT_FOUND;
        }
        if (image_map->states[index] == TPXL_ENTRY_OCCUPIED) {

            if (image_map->entries[index].id == frame_id) {
                *out_entry = &image_map->entries[index];
                return TPXL_OK;
            }
        }
        index = (index + 1) % TPXL_IMAGE_MAP_CAPACITY;
    }

    return TPXL_MAP_NOT_FOUND;
}

void tpxl_image_map_destroy(TpxlImageMap* image_map) {

    assert(image_map);

    for (size_t i = 0; i < TPXL_IMAGE_MAP_CAPACITY; i++) {
        if (image_map->states[i] == TPXL_ENTRY_OCCUPIED) {
            free(image_map->entries[i].data); 
        }
    }

    *image_map = (TpxlImageMap){0};
}