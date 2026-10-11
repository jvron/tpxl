#ifndef TPXL_SIXEL_IMAGE_MAP
#define TPXL_SIXEL_IMAGE_MAP

#include <stdint.h>
#include <stdio.h>

#include "tpxl/type.h"

#define TPXL_SIXEL_IMAGE_MAP_CAPACITY 256

typedef enum {
    TPXL_ENTRY_EMPTY = 0,
    TPXL_ENTRY_OCCUPIED,
    TPXL_ENTRY_DELETED,
} TpxlSixelEntryState;

typedef struct {
    char* data;
    size_t size;
    uint32_t id;
    uint32_t row;
    uint32_t column;
    uint32_t rows;
    uint32_t columns;
} TpxlSixelImage;

typedef struct {
    TpxlSixelImage entries[TPXL_SIXEL_IMAGE_MAP_CAPACITY];
    TpxlSixelEntryState states[TPXL_SIXEL_IMAGE_MAP_CAPACITY];
    size_t count;
} TpxlSixelImageMap;

TpxlResult tpxl_sixel_image_map_init(TpxlSixelImageMap* image_map);
TpxlResult tpxl_sixel_image_map_insert(TpxlSixelImageMap* image_map, TpxlSixelImage* sixel_image, uint32_t frame_id);
TpxlResult tpxl_sixel_image_map_remove(TpxlSixelImageMap* image_map, uint32_t frame_id);
TpxlResult tpxl_sixel_image_map_get(TpxlSixelImageMap* image_map, uint32_t frame_id, TpxlSixelImage** out_sixel_image);
void tpxl_sixel_image_map_destroy(TpxlSixelImageMap* image_map);

#endif