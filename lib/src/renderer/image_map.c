#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "tpxl/type.h"

#include "image_map.h"

static inline uint32_t tpxl_hash_u32(uint32_t key) {
    return key * 2654435761u;
}

TpxlResult tpxl_image_map_init(TpxlImageMap* image_map) {

    assert(image_map);

    image_map->capacity = 256;
    image_map->count = 0;

    image_map->entries = malloc(image_map->capacity * sizeof(TpxlImageEntry));
    image_map->states = calloc(image_map->capacity, sizeof(TpxlEntryState));

    if (!image_map->entries || !image_map->states) {
        free(image_map->entries);
        free(image_map->states);
        *image_map = (TpxlImageMap){0};
        return TPXL_OUT_OF_MEMORY;
    }

    return TPXL_OK;
}


TpxlResult tpxl_image_map_insert(TpxlImageMap* image_map, TpxlImageEntry* entry, uint32_t frame_id) {

    assert(image_map && entry);

    if (image_map->count >= image_map->capacity) {
        size_t required = image_map->capacity * 2;

        TpxlImageEntry* entries = malloc(required * sizeof(TpxlImageEntry));
        TpxlEntryState* states = calloc(required, sizeof(TpxlEntryState));

        if (!entries || !states) {
            free(entries);
            free(states);
            return TPXL_OUT_OF_MEMORY;
        }

        for (size_t i = 0; i < image_map->capacity; i++) {
            if (image_map->states[i] != TPXL_ENTRY_OCCUPIED) {
                continue;
            }

            size_t new_idx = tpxl_hash_u32(image_map->entries[i].id) % required;

            for (size_t probe = 0; probe < required; probe++) {
                if (states[new_idx] == TPXL_ENTRY_EMPTY) {
                    entries[new_idx] = image_map->entries[i];
                    states[new_idx] = TPXL_ENTRY_OCCUPIED;
                    break;
                }

                new_idx = (new_idx + 1) % required;
            }
        }

        free(image_map->entries);
        free(image_map->states);

        image_map->capacity = required;
        image_map->entries = entries;
        image_map->states = states;
    }

    size_t index = tpxl_hash_u32(frame_id) % image_map->capacity;

    int deleted_index = -1;

    for (size_t probe = 0; probe < image_map->capacity; probe++) {

        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {

            if (deleted_index >= 0) {
                break;
            } 
  
            image_map->entries[index] = *entry;
            image_map->states[index] = TPXL_ENTRY_OCCUPIED;
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
        index = (index + 1) % image_map->capacity;
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

    size_t index = tpxl_hash_u32(frame_id) % image_map->capacity;

    for (size_t probe = 0; probe < image_map->capacity; probe++) {
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
        index = (index + 1) % image_map->capacity;
    }

    return TPXL_MAP_NOT_FOUND;
}

TpxlResult tpxl_image_map_get(TpxlImageMap* image_map, uint32_t frame_id, TpxlImageEntry** out_entry) {

    assert(image_map && out_entry);

    if (image_map->count == 0) {
        return TPXL_MAP_EMPTY;
    }

    size_t index = tpxl_hash_u32(frame_id) % image_map->capacity;

    for (size_t probe = 0; probe < image_map->capacity; probe++) {
        if (image_map->states[index] == TPXL_ENTRY_EMPTY) {
            return TPXL_MAP_NOT_FOUND;
        }
        if (image_map->states[index] == TPXL_ENTRY_OCCUPIED) {

            if (image_map->entries[index].id == frame_id) {
                *out_entry = &image_map->entries[index];
                return TPXL_OK;
            }
        }
        index = (index + 1) % image_map->capacity;
    }

    return TPXL_MAP_NOT_FOUND;
}

void tpxl_image_map_destroy(TpxlImageMap* image_map) {

    assert(image_map);

    for (size_t i = 0; i < image_map->capacity; i++) {
        if (image_map->states[i] == TPXL_ENTRY_OCCUPIED) {
            free(image_map->entries[i].data); 
        }
    }

    free(image_map->entries);
    free(image_map->states);

    *image_map = (TpxlImageMap){0};
}