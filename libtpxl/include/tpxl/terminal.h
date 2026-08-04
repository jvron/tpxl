#ifndef TPXL_TERMINAL_H
#define TPXL_TERMINAL_H

#include <stdint.h>

#include "type.h"

typedef struct {
    uint32_t rows;
    uint32_t columns;

    uint32_t cell_width;
    uint32_t cell_height;

    uint32_t pixel_width;
    uint32_t pixel_height;

} TpxlTerminal;

TpxlResult tpxl_init_terminal(TpxlTerminal* terminal);
TpxlResult tpxl_query_terminal(TpxlTerminal* terminal);

#endif
