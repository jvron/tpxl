#ifndef TPXL_TERMINAL_H
#define TPXL_TERMINAL_H

#include <stdint.h>
#include <termios.h>
#include <stdbool.h>

#include "type.h"

typedef struct {

    struct termios original_termios;
    bool initialized;

    uint32_t rows;
    uint32_t columns;

    uint32_t cell_width;
    uint32_t cell_height;

    uint32_t pixel_width;
    uint32_t pixel_height;

    uint32_t cursor_row;
    uint32_t cursor_column;

} TpxlTerminal;

TpxlResult tpxl_init_terminal(TpxlTerminal* terminal);
TpxlResult tpxl_get_cursor_position(uint32_t* row, uint32_t* column);
TpxlResult tpxl_query_terminal(TpxlTerminal* terminal);
TpxlResult tpxl_shutdown_terminal(TpxlTerminal* terminal);

#endif
