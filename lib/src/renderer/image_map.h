#ifndef TPXL_IMAGE_MAP_H
#define TPXL_IMAGE_MAP_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "tpxl/type.h"

typedef enum {
    TPXL_ENTRY_EMPTY = 0,
    TPXL_ENTRY_OCCUPIED,
    TPXL_ENTRY_DELETED,
} TpxlEntryState;

typedef struct {
    char* data;
    size_t size;
    uint32_t id;
    uint32_t row;
    uint32_t column;
    uint32_t rows;
    uint32_t columns;
    bool displayed;
} TpxlImageEntry;

typedef struct {
    TpxlImageEntry* entries;
    TpxlEntryState* states;
    size_t count;
    size_t capacity;
} TpxlImageMap;

TpxlResult tpxl_image_map_init(TpxlImageMap* image_map);
TpxlResult tpxl_image_map_insert(TpxlImageMap* image_map, TpxlImageEntry* entry);
TpxlResult tpxl_image_map_remove(TpxlImageMap* image_map, uint32_t frame_id);
TpxlResult tpxl_image_map_get(TpxlImageMap* image_map, uint32_t frame_id, TpxlImageEntry** out_entry);
void tpxl_image_map_destroy(TpxlImageMap* image_map);

#endif