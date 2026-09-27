#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/terminal.h"

#ifdef __WIN32

#include <windows.h>

static DWORD original_console_mode;

TpxlResult tpxl_init_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    *terminal = (TpxlTerminal){0};

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (input == INVALID_HANDLE_VALUE) {
        return TPXL_IO_ERROR;
    }

    if (!GetConsoleMode(input, &original_console_mode)) {
        return TPXL_IO_ERROR;
    }

    DWORD tpxl_mode = original_console_mode;

    tpxl_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    if (!SetConsoleMode(input, tpxl_mode)) {
        return TPXL_IO_ERROR;
    }

    DWORD output_mode;

    if (!GetConsoleMode(output, &output_mode)) {
        return TPXL_IO_ERROR;
    }

    output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(output, output_mode)) {
        return TPXL_IO_ERROR;
    }

    terminal->initialized = true;

    return TPXL_OK;
}

TpxlResult tpxl_get_cursor_position(uint32_t* row, uint32_t* column) {

    if (!row || !column) {
        return TPXL_INVALID_ARGUMENT;
    }

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output == INVALID_HANDLE_VALUE) {
        return TPXL_IO_ERROR;
    }

    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(output, &info)) {
        return TPXL_IO_ERROR;
    }

    *row = (uint32_t)info.dwCursorPosition.Y + 1;
    *column = (uint32_t)info.dwCursorPosition.X + 1;

    return TPXL_OK;
}

TpxlResult tpxl_query_terminal(TpxlTerminal* terminal) {
    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output == INVALID_HANDLE_VALUE) {
        return TPXL_IO_ERROR;
    }

    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(output, &info)) {
        return TPXL_IO_ERROR;
    }

    terminal->columns = (uint32_t)(info.srWindow.Right - info.srWindow.Left + 1);
    terminal->rows = (uint32_t)(info.srWindow.Bottom - info.srWindow.Top + 1);

    if (!terminal->columns || !terminal->rows) {
        return TPXL_IO_ERROR;
    }

    HWND window = GetConsoleWindow();

    if (!window) {
        return TPXL_IO_ERROR;
    }

    RECT rect;

    if (!GetClientRect(window, &rect)) {
        return TPXL_IO_ERROR;
    }

    terminal->pixel_width = (uint32_t)(rect.right - rect.left);
    terminal->pixel_height = (uint32_t)(rect.bottom - rect.top);

    if (!terminal->pixel_width || !terminal->pixel_height) {
        return TPXL_IO_ERROR;
    }

    terminal->cell_width = terminal->pixel_width / terminal->columns;
    terminal->cell_height = terminal->pixel_height / terminal->rows;

    return tpxl_get_cursor_position(&terminal->cursor_row, &terminal->cursor_column);
}

TpxlResult tpxl_shutdown_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    if (!terminal->initialized) {
        return TPXL_OK;
    }

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (input == INVALID_HANDLE_VALUE) {
        return TPXL_IO_ERROR;
    }

    if (!SetConsoleMode(input, original_console_mode)) {
        return TPXL_IO_ERROR;
    }

    *terminal = (TpxlTerminal){0};

    return TPXL_OK;
}

#else 

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios original_termios;

TpxlResult tpxl_init_terminal(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    *terminal = (TpxlTerminal){0};

    // save current terminal settings
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        return TPXL_IO_ERROR;
    }

    struct termios tpxl_termios = original_termios;

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

    fprintf(stdout, "\033[6n");
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

    if (parsed_row <= 0 || parsed_column <= 0) {
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

    if (!window_size.ws_row || !window_size.ws_col) {
        return TPXL_IO_ERROR;
    }   

    terminal->rows = window_size.ws_row;
    terminal->columns = window_size.ws_col;

    if (window_size.ws_xpixel && window_size.ws_ypixel) {
        terminal->pixel_width = window_size.ws_xpixel;
        terminal->pixel_height = window_size.ws_ypixel;
    } else {
        fprintf(stdout, "\033[14t");
        fflush(stdout);

        char buffer[32];
        size_t i = 0;
        char c;

        while (i < sizeof(buffer) - 1) {

            if (read(STDIN_FILENO, &c, 1) != 1) {
                return TPXL_IO_ERROR;
            }

            buffer[i++] = c;

            if (c == 't') {
                break;
            }
        }

        buffer[i] = '\0';

        int height;
        int width;

        if (sscanf(buffer, "\033[4;%d;%dt", &height, &width) != 2) {
            return TPXL_IO_ERROR;
        }

        if (height <= 0 || width <= 0) {
            return TPXL_IO_ERROR;
        }

        terminal->pixel_height = (uint32_t)height;
        terminal->pixel_width = (uint32_t)width;
    }

    terminal->cell_width = terminal->pixel_width / terminal->columns;
    terminal->cell_height = terminal->pixel_height / terminal->rows;
    
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
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_termios) == -1) {
        return TPXL_IO_ERROR;
    }

    *terminal = (TpxlTerminal){0};

    return TPXL_OK;
}

#endif

TpxlResult tpxl_move_cursor(uint32_t row, uint32_t column) {

    if(fprintf(stdout, "\033[%u;%uH", row, column) < 0) {
        return TPXL_IO_ERROR;
    }

    return TPXL_OK;
}
