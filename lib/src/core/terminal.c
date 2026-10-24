#include <stdio.h>

#include "tpxl/type.h"
#include "tpxl/terminal.h"

#ifdef __WIN32

#include <windows.h>

static DWORD original_console_mode;

TpxlResult tpxl_terminal_init(TpxlTerminal* terminal) {

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

    tpxl_mode &= ~(ENABLE_LINE_INPUT |
                   ENABLE_ECHO_INPUT |
                   ENABLE_PROCESSED_INPUT);

    tpxl_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    if (!SetConsoleMode(input, tpxl_mode)) {
        return TPXL_IO_ERROR;
    }

    DWORD output_mode;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

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

TpxlResult tpxl_terminal_get_cursor_position(uint32_t* row, uint32_t* column) {

    if (!row || !column) {
        return TPXL_INVALID_ARGUMENT;
    }

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output == INVALID_HANDLE_VALUE || output == NULL) {
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

TpxlResult tpxl_terminal_query(TpxlTerminal* terminal) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (output == INVALID_HANDLE_VALUE || output == NULL ||
        input == INVALID_HANDLE_VALUE || input == NULL) {
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

    static const char query[] = "\033[14t";
    DWORD written = 0;

    if (!WriteFile(output, query, sizeof(query) - 1, &written, NULL) ||
        written != sizeof(query) - 1) {
        return TPXL_IO_ERROR;
    }

    char buffer[64];
    size_t length = 0;
    bool has_response = false;
    DWORD start = GetTickCount();

    while (length < sizeof(buffer) - 1 && GetTickCount() - start < 500) {
        DWORD available = 0;

        if (!GetNumberOfConsoleInputEvents(input, &available)) {
            return TPXL_IO_ERROR;
        }

        if (available == 0) {
            Sleep(1);
            continue;
        }

        INPUT_RECORD records[16];
        DWORD count = 0;

        if (!ReadConsoleInputA(input, records, 16, &count)) {
            return TPXL_IO_ERROR;
        }

        for (DWORD i = 0; i < count; ++i) {
            if (records[i].EventType != KEY_EVENT ||
                !records[i].Event.KeyEvent.bKeyDown) {
                continue;
            }

            char c = records[i].Event.KeyEvent.uChar.AsciiChar;

            if (!c) {
                continue;
            }

            buffer[length++] = c;

            if (c == 't') {
                has_response = true;
                break;
            }
        }

        if (has_response) {
            break;
        }
    }

    buffer[length] = '\0';

    uint32_t height = 0;
    uint32_t width = 0;

    if (sscanf(buffer, "\033[4;%u;%ut", &height, &width) != 2) {
        return TPXL_IO_ERROR;
    }

    if (height <= 0 || width <= 0) {
        return TPXL_IO_ERROR;
    }

    terminal->pixel_width = width;
    terminal->pixel_height = height;

    terminal->cell_width = width / terminal->columns;
    terminal->cell_height = height / terminal->rows;

    if (!terminal->cell_width || !terminal->cell_height) {
        return TPXL_IO_ERROR;
    }

    return tpxl_terminal_get_cursor_position(&terminal->cursor_row, &terminal->cursor_column);
}

TpxlResult tpxl_terminal_shutdown(TpxlTerminal* terminal) {

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

TpxlResult tpxl_terminal_init(TpxlTerminal* terminal) {

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

TpxlResult tpxl_terminal_get_cursor_position(uint32_t* row, uint32_t* column) {

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

TpxlResult tpxl_terminal_query(TpxlTerminal* terminal) {

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

        uint32_t height = 0;
        uint32_t width = 0;

        if (sscanf(buffer, "\033[4;%u;%ut", &height, &width) != 2) {
            return TPXL_IO_ERROR;
        }

        if (height <= 0 || width <= 0) {
            return TPXL_IO_ERROR;
        }

        terminal->pixel_height = height;
        terminal->pixel_width = width;
    }

    terminal->cell_width = terminal->pixel_width / terminal->columns;
    terminal->cell_height = terminal->pixel_height / terminal->rows;

    return tpxl_terminal_get_cursor_position(&terminal->cursor_row, &terminal->cursor_column);



    return TPXL_OK;
}

TpxlResult tpxl_terminal_shutdown(TpxlTerminal* terminal) {

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

TpxlResult tpxl_terminal_move_cursor(uint32_t row, uint32_t column) {

    if(fprintf(stdout, "\033[%u;%uH", row, column) < 0) {
        return TPXL_IO_ERROR;
    }

    return TPXL_OK;
}

TpxlResult tpxl_terminal_ensure_rows(TpxlTerminal* terminal, uint32_t required_rows) {

    if (!terminal) {
        return TPXL_INVALID_ARGUMENT;
    }

    uint32_t available_rows = terminal->rows - terminal->cursor_row;

    if (available_rows >= required_rows) {
        return TPXL_OK;
    }

    uint32_t scroll_rows = required_rows - available_rows;

    fprintf(stdout, "\033[%uS", scroll_rows);
    fprintf(stdout, "\033[%uA", scroll_rows);
    fflush(stdout);

    return tpxl_terminal_get_cursor_position(&terminal->cursor_row, &terminal->cursor_column);
}
