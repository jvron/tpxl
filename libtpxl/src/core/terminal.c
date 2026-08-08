#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "tpxl/type.h"
#include "tpxl/terminal.h"

TpxlResult tpxl_init_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    terminal->rows = 0;
    terminal->columns = 0;
    terminal->cell_width = 0;
    terminal->cell_height = 0;
    terminal->pixel_width = 0;
    terminal->pixel_height = 0;

    return TPXL_OK;
}

TpxlResult tpxl_get_cursor_position(uint32_t* row, uint32_t* column) {

    char buffer[32];
    size_t i = 0;
    char c;

    printf("\033[6n");
    fflush(stdout);

    while (i < sizeof(buffer) - 1) {

        if (read(STDIN_FILENO, &c, 1) != 1) {
            return TPXL_IO_ERROR;
        }

        buffer[i++] = c;

        if (c == 'R') {
            break;
        }
    }

    buffer[i] = '\0';

    if (sscanf(buffer, "\033[%d;%dR", row, column) != 2) {
        return TPXL_IO_ERROR;
    }

    return TPXL_OK;
}

TpxlResult tpxl_query_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    struct winsize window_size;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == -1) {
        return TPXL_IO_ERROR;
    }

    terminal->rows = window_size.ws_row;
    terminal->columns = window_size.ws_col;
    terminal->pixel_width = window_size.ws_xpixel;
    terminal->pixel_height = window_size.ws_ypixel;

    if (terminal->rows && terminal->columns && terminal->pixel_width && terminal->pixel_height) {
        terminal->cell_width = terminal->pixel_width / terminal->columns;
        terminal->cell_height = terminal->pixel_height / terminal->rows;
    }

    TpxlResult result = tpxl_get_cursor_position(&terminal->cursor_row, &terminal->cursor_column);

    if (result != TPXL_OK) {
        return result;
    }

    return TPXL_OK;
}
