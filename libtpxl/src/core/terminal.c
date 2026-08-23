#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "tpxl/type.h"
#include "tpxl/terminal.h"

TpxlResult tpxl_init_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    *terminal = (TpxlTerminal){0};

    // save current terminal settings
    if (tcgetattr(STDIN_FILENO, &terminal->original_termios) == -1) {
        return TPXL_IO_ERROR;
    }

    struct termios tpxl_termios = terminal->original_termios;

    // disable canonical mode and echo
    tpxl_termios.c_lflag &= ~(ICANON | ECHO);

    // apply modified settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tpxl_termios) == -1) {
        return TPXL_IO_ERROR;
    }

    terminal->initialized = true;

    return TPXL_OK;
}

TpxlResult tpxl_get_cursor_position(uint32_t* row, uint32_t* column) {

    if (!row || !column) {
        return TPXL_INVALID_ARGUMENT;
    }

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

    int parsed_row;
    int parsed_column;

    if (sscanf(buffer, "\033[%d;%dR", &parsed_row, &parsed_column) != 2) {
        return TPXL_IO_ERROR;
    }

    *row = (uint32_t)parsed_row;
    *column = (uint32_t)parsed_column;

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

TpxlResult tpxl_shutdown_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!terminal->initialized) {
        return TPXL_OK;
    }

    // set back original settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal->original_termios) == -1) {
        return TPXL_IO_ERROR;
    }

    *terminal = (TpxlTerminal){0};

    return TPXL_OK;
}
