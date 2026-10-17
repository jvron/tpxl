#ifndef TPXL_IMAGE_MAP_H
#define TPXL_IMAGE_MAP_H

#include <stdint.h>
#include <stdio.h>

#include "tpxl/type.h"

#define TPXL_IMAGE_MAP_CAPACITY 256

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
} TpxlImageMapEntry;

typedef struct {
    TpxlImageMapEntry entries[TPXL_IMAGE_MAP_CAPACITY];
    TpxlEntryState states[TPXL_IMAGE_MAP_CAPACITY];
    size_t count;
} TpxlImageMap;

TpxlResult tpxl_image_map_init(TpxlImageMap* image_map);
TpxlResult tpxl_image_map_insert(TpxlImageMap* image_map, TpxlImageMapEntry* sixel_image, uint32_t frame_id);
TpxlResult tpxl_image_map_remove(TpxlImageMap* image_map, uint32_t frame_id);
TpxlResult tpxl_image_map_get(TpxlImageMap* image_map, uint32_t frame_id, TpxlImageMapEntry** out_entry);
void tpxl_image_map_destroy(TpxlImageMap* image_map);

#endif